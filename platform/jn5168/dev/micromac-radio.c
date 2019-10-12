/*
 * Copyright (c) 2014, NXP & Swedish Institute of Computer Science
 * All rights reserved.
 */

/**
 * \file
 *         NXP JN5168 radio driver using MMAC interface
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 *
 */
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/packetbuf.h"
#include "net/rime/rimestats.h"
#include "net/netstack.h"
#include "net/mac/frame802154.h"
#include "lib/crc16.h"
#include "lib/ringbufindex.h"

#include "HardwareApi/Include/AppHardwareApi.h"
#include "AppApi/Include/AppApi.h"
#include "MMAC/Include/MMAC.h"
#include "micromac-radio.h"

/* Perform CRC check for received packets in SW,
 * since we use Phy mode which does not calculate CRC in HW
 * PS: the current documentation state that CRC is checked
 * in Phy receive but not transmit. This is incorrect according to Sheffield */
#define CRC_SW 1

/* Shall we accept corrupted packets? */
#if MICROMAC_CONF_ACCEPT_FCS_ERROR
#define MICROMAC_ACCEPT_FCS_ERROR 1
#define MICROMAC_CONF_RX_FCS_ERROR E_MMAC_RX_ALLOW_FCS_ERROR
#else
#define MICROMAC_ACCEPT_FCS_ERROR 0
#define MICROMAC_CONF_RX_FCS_ERROR E_MMAC_RX_NO_FCS_ERROR
#endif

/* XXX JPT functions disabled for a potential compatibility issue with MMAC */
#ifndef ENABLE_JPT
#define ENABLE_JPT 0
#endif /* ENABLE_JPT */

#if ENABLE_JPT
#include "Utilities/Include/JPT.h"
#endif /* ENABLE_JPT */

/* STRESS_TEST:
 * wait until ringbuf is full and then poll micromac_process
 * to stress-test the overflow protection mechanism */
#define STRESS_TEST 0

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#define MIN(A,B)                ( ( ( A ) < ( B ) ) ? ( A ) : ( B ) )

#ifdef RADIO_BUF_CONF_NUM
#define RADIO_BUF_NUM RADIO_BUF_CONF_NUM
#else
#define RADIO_BUF_NUM 2
#endif /* RADIO_BUF_CONF_NUM */

/* Enable test-mode pins output on dev-kit */
#ifndef RADIO_TEST_MODE
#define RADIO_TEST_MODE  RADIO_TEST_MODE_DISABLED
#endif /* RADIO_TEST_MODE */

#define CHECKSUM_LEN 2

#ifndef RF_CHANNEL
#define RF_CHANNEL (26)
#endif

#ifndef RF_POWER
#define RF_POWER (MICROMAC_RADIO_TXPOWER_MAX)
#endif

struct output_config {
  int8_t power;
  uint8_t config;
};
/* TODO OUTPUT_POWER_ values are adhoc. Check datasheet */
static const struct output_config output_power[] = {
  {  0, 3 }, /* 0xff */
  { -7, 2 }, /* 0xef */
  {-15,  1 }, /* 0xe7 */
  {-25,  0 }, /* 0xe3 */
};

#define OUTPUT_NUM (sizeof(output_power) / sizeof(struct output_config))
#define OUTPUT_POWER_MAX   0
#define OUTPUT_POWER_MIN -25

/* XXX RSSI_THR has an arbitrary value */
/* an integer between 0 and 255, used only with micromac_radio_cca() */
#ifndef RSSI_THR
#define RSSI_THR 127
#endif /* RSSI_THR */

#if MICROMAC_RADIO_CONF_AUTOACK
#define MMAC_RX_AUTO_ACK_CONF E_MMAC_RX_USE_AUTO_ACK
#define MMAC_TX_AUTO_ACK_CONF E_MMAC_TX_USE_AUTO_ACK
#pragma "MICROMAC_RADIO_CONF_AUTOACK enabled"
#else
#define MMAC_RX_AUTO_ACK_CONF E_MMAC_RX_NO_AUTO_ACK
#define MMAC_TX_AUTO_ACK_CONF E_MMAC_TX_NO_AUTO_ACK
#endif /* MICROMAC_RADIO_CONF_AUTOACK */

#ifndef IEEE802154_PANID
# ifdef IEEE802154_CONF_PANID
#   define IEEE802154_PANID           IEEE802154_CONF_PANID
# else
#   define IEEE802154_PANID           0xABCD
# endif
#endif

#define WITH_SEND_CCA 0


extern unsigned char node_mac[8];

/* Local variables */

/* LQI is derived from the RF energy level and scaled
 * to the range 0 (lowest) to 255
 * (highest energy level that the device can determine) */
static uint8_t micromac_radio_last_lqi = 0;

/* modem signal quality (MSQ) value
 * based on the error detection within the correlator in the PHY */
static uint8_t micromac_radio_last_msq = 0;

signed char radio_last_rssi;
uint8_t radio_last_correlation;

volatile uint16_t radio_last_rx_crc;
volatile uint8_t radio_last_rx_crc_ok;

volatile static uint16_t my_pan_id = (uint16_t) IEEE802154_PANID;
volatile static uint16_t my_short_address = (uint16_t) 0xffff;
static tsExtAddr my_mac_address;

static volatile uint32_t radio_ref_time = 0;
static volatile rtimer_clock_t rtimer_ref_time = 0;

/* A flag to keep the radio always on */
static uint8_t volatile always_on = 0;

/* did we miss a request to rurn the radio on due to overflow? */
static uint8_t volatile missed_radio_on_request = 0;

/* A flag to enable or disable RADIO interrupt */
static uint8_t volatile interrupt_enabled = 0;

static uint8_t volatile address_decoding_enabled = 1;

/* Configured radio channel */
static int channel;

/* an integer between 0 and 255, used only with micromac_radio_cca() */
static volatile uint8_t cca_thershold = RSSI_THR;

static volatile uint8_t tx_in_progress = 0;

static volatile uint8_t rx_in_progress = 0;

/* pending packets? */
static volatile uint8_t pending = 0;

/* TX frame buffer */
static tsPhyFrame tx_frame_buffer;

/* RX frame buffer */
static tsPhyFrame *rx_frame_buffer;

/* Frame buffer pointer to read from */
static tsPhyFrame *input_frame_buffer = NULL;

/* Ringbuffer for received packets in interrupt enabled mode */
static struct ringbufindex input_ringbuf;
static tsPhyFrame input_array[RADIO_BUF_NUM];

/* SFD timestamp in RTIMER ticks */
static volatile uint32_t last_packet_timestamp = 0;

/* Local functions prototypes */
static void on(void);
static void off(void);
static int micromac_radio_is_packet_for_us(uint8_t* p, int len);
/*---------------------------------------------------------------------------*/
PROCESS(micromac_radio_process, "micromac_radio_driver");
/*---------------------------------------------------------------------------*/
static uint8_t locked, lock_on, lock_off;
#define GET_LOCK() (locked++)
static void
RELEASE_LOCK(void)
{
  if(locked == 1) {
    if(lock_on) {
      on();
      lock_on = 0;
    }
    if(lock_off) {
      off();
      lock_off = 0;
    }
  }
  locked--;
}
/*---------------------------------------------------------------------------*/
void
micromac_radio_sfd_sync(void)
{
  radio_ref_time = u32MMAC_GetTime();
  rtimer_ref_time = RTIMER_NOW();
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
micromac_radio_read_sfd_timer(void)
{
  /* Save SFD timestamp
   * We need to convert from radio timer to RTIMER */

  /* One way of doing it */
  last_packet_timestamp = RTIMER_NOW()
      - RADIO_TO_RTIMER((uint32_t )(u32MMAC_GetTime() - u32MMAC_GetRxTime()));
  /* Another way of doing it */
  /* last_packet_timestamp = RADIO_TO_RTIMER(u32MMAC_GetRxTime() - radio_ref_time) + rtimer_ref_time; */

  return last_packet_timestamp;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_init(void)
{
  uint8_t u8TxAttempts = 1, /* 1 transmission attempt without ACK */
  u8MinBE = 0, /* min backoff exponent */
  u8MaxBE = 0, /* max backoff exponent */
  u8MaxBackoffs = 0; /* backoff before aborting */
  uint32_t jpt_ver = 0; /* if 0 then init failed */
  if(locked) {
    return 0;
  }
  GET_LOCK();
  tx_in_progress = 0;

#if ENABLE_JPT
  jpt_ver = u32JPT_Init();
#endif /* ENABLE_JPT */

  vMMAC_Enable();

  if(!interrupt_enabled) {
    vMMAC_EnableInterrupts(NULL);
    vMMAC_ConfigureInterruptSources(0);
  } else {
    vMMAC_EnableInterrupts(&micromac_radio_interrupt);
  }

  vMMAC_ConfigureRadio();
  channel = RF_CHANNEL;
  vMMAC_SetChannel(channel);

#if ENABLE_JPT
  vJPT_RadioSetPower(RF_POWER);
#endif /* ENABLE_JPT */

  vMMAC_GetMacAddress(&my_mac_address);
  /* short addresses are disabled by default */
  my_short_address = (uint16_t) my_mac_address.u32L;
  vMMAC_SetRxAddress(my_pan_id, my_short_address, &my_mac_address);

  /* these parameters should disable hardware backoff, but still enable autoACK processing and TX CCA */
  vMMAC_SetTxParameters(u8TxAttempts, u8MinBE, u8MaxBE, u8MaxBackoffs);
  vMMAC_SetCutOffTimer(0, FALSE);

  RELEASE_LOCK();

  /* initialize ringbuffer and first input packet pointer */
  ringbufindex_init(&input_ringbuf, RADIO_BUF_NUM);
  /* get pointer to next input slot */
  int put_index = ringbufindex_peek_put(&input_ringbuf);
  if(put_index == -1) {
    rx_frame_buffer = NULL;
    printf("micromac_radio init: Not enough RADIO_BUF_NUM %d. Failing!\n",
        RADIO_BUF_NUM);
    off();
    return 0;
  } else {
    rx_frame_buffer = &input_array[put_index];
  }
  if(!interrupt_enabled) {
    input_frame_buffer = rx_frame_buffer;
  }
  process_start(&micromac_radio_process, NULL);
  printf(
      "micromac_radio init: RXAddress %04x == %08x.%08x @ PAN %04x, channel: %d\n",
      my_short_address, my_mac_address.u32H, my_mac_address.u32L, my_pan_id,
      channel);

#if RADIO_TEST_MODE == RADIO_TEST_MODE_HIGH_PWR
#include "HardwareApi/Include/PeripheralRegs.h"
  /* Enable high power mode.
   * In this mode DIO2 goes high during RX
   * and DIO3 goes high during TX
   **/
  vREG_SysWrite(REG_SYS_PWR_CTRL,
      u32REG_SysRead(REG_SYS_PWR_CTRL)
      | REG_SYSCTRL_PWRCTRL_RFRXEN_MASK
      | REG_SYSCTRL_PWRCTRL_RFTXEN_MASK);
#elif RADIO_TEST_MODE == RADIO_TEST_MODE_ADVANCED
#include "HardwareApi/Include/PeripheralRegs.h"
  /* output internal radio status on IO pins.
   * See Chris@NXP email */
  vREG_SysWrite(REG_SYS_PWR_CTRL,
      u32REG_SysRead(REG_SYS_PWR_CTRL) | (1UL << 26UL));
#endif /* TEST_MODE */

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
micromac_radio_phy_rx(void)
{
  /* The only valid automatic option is FCS check.
   * No address matching or frame decoding */
  if(rx_frame_buffer != NULL) {
    vMMAC_StartPhyReceive(rx_frame_buffer,
      (uint16_t) (E_MMAC_RX_START_NOW
                 | MICROMAC_CONF_RX_FCS_ERROR) /* means: reject FCS errors */
      );
  } else {
    missed_radio_on_request = 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
on(void)
{
  /* TODO not needed since we are using radio timer directly
   * micromac_radio_sfd_sync();*/
  micromac_radio_phy_rx();
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_raw_rx_on(void)
{
  GET_LOCK();
  rx_in_progress = 1;
  micromac_radio_phy_rx();
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
off(void)
{
//  rx_in_progress = 0;
//  tx_in_progress = 0;
  /* TODO only needed when using scheduled/delayed RF functions
   * vMMAC_SetCutOffTimer(0, FALSE);*/
  vMMAC_RadioOff();
  /* TODO PROTOCOL shutdown... to shutdown the circuit instead. */
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_transmit(unsigned short payload_len)
{
  PRINTF("micromac_radio_transmit\n");
  if(tx_in_progress) {
    PRINTF("micromac_radio_transmit: RADIO_TX_COLLISION\n");
    return RADIO_TX_COLLISION;
  }
  GET_LOCK();
  tx_in_progress = 1;
  /* TODO no auto ack in Pyh mode, remove it here and constuct ack in interrupt */
  vMMAC_StartPhyTransmit(&tx_frame_buffer,
      E_MMAC_TX_START_NOW | MMAC_TX_AUTO_ACK_CONF | E_MMAC_TX_NO_CCA);
  /* TODO should this be removed? */
  BUSYWAIT_UNTIL(u32MMAC_PollInterruptSource(E_MMAC_INT_TX_COMPLETE),
      MAX_PACKET_DURATION);
  tx_in_progress = 0;
  int ret = RADIO_TX_ERR;
  uint32_t tx_error = u32MMAC_GetTxErrors();
  if(tx_error == 0) {
    ret = RADIO_TX_OK;
    RIMESTATS_ADD(acktx);
  } else if(tx_error & E_MMAC_TXSTAT_ABORTED) {
    ret = RADIO_TX_ERR;
    RIMESTATS_ADD(sendingdrop);
  } else if(tx_error & E_MMAC_TXSTAT_CCA_BUSY) {
    ret = RADIO_TX_COLLISION;
    RIMESTATS_ADD(contentiondrop);
  } else if(tx_error & E_MMAC_TXSTAT_NO_ACK) {
    ret = RADIO_TX_NOACK;
    RIMESTATS_ADD(noacktx);
  }
  RELEASE_LOCK();
  return ret;
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_prepare(const void *payload, unsigned short payload_len)
{
  uint8_t i;
  uint16_t checksum;

  RIMESTATS_ADD(lltx);

  if(tx_in_progress) {
    return 1;
  }
  if(payload_len > MICROMAC_RADIO_MAX_PACKET_LEN || payload == NULL) {
    return 1;
  }
  GET_LOCK();
  /* copy payload to (soft) tx buffer */
  memset(&tx_frame_buffer, 0, sizeof(tx_frame_buffer));
  //  memcpy(&(tx_frame_buffer.uPayload.au8Byte), payload, payload_len);
  for(i = 0; i < payload_len; i++) {
    tx_frame_buffer.uPayload.au8Byte[i] = ((uint8_t*) payload)[i];
  }
  checksum = crc16_data(payload, payload_len, 0);
  tx_frame_buffer.uPayload.au8Byte[i++] = checksum;
  tx_frame_buffer.uPayload.au8Byte[i++] = (checksum >> 8) & 0xff;
  tx_frame_buffer.u8PayloadLength = payload_len + CHECKSUM_LEN;
  RELEASE_LOCK();
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_send(const void *payload, unsigned short payload_len)
{
  micromac_radio_prepare(payload, payload_len);
  return micromac_radio_transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_get_transmission_status(void)
{
  int ret = RADIO_TX_ERR;
  uint32_t tx_error = u32MMAC_GetTxErrors();
  if(tx_error == 0) {
    ret = RADIO_TX_OK;
    RIMESTATS_ADD(acktx);
  } else if(tx_error & E_MMAC_TXSTAT_ABORTED) {
    ret = RADIO_TX_ERR;
    RIMESTATS_ADD(sendingdrop);
  } else if(tx_error & E_MMAC_TXSTAT_CCA_BUSY) {
    ret = RADIO_TX_COLLISION;
    RIMESTATS_ADD(contentiondrop);
  } else if(tx_error & E_MMAC_TXSTAT_NO_ACK) {
    ret = RADIO_TX_NOACK;
    RIMESTATS_ADD(noacktx);
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_off(void)
{
  GET_LOCK();
  off();
  rx_in_progress = 0;
  tx_in_progress = 0;
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_on(void)
{
  GET_LOCK();
  rx_in_progress = 1;
  on();
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_get_channel(void)
{
  return channel;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_set_channel(int c)
{
  GET_LOCK();
  channel = c;
  vMMAC_SetChannel(channel);
  RELEASE_LOCK();
  return 1;
}
/*---------------------------------------------------------------------------*/
void
micromac_radio_set_pan_addr(unsigned pan, unsigned addr,
    const uint8_t *ieee_addr)
{
  GET_LOCK();
  my_pan_id = pan;
  my_short_address = addr;
  /* copy ieee_addr to my_mac_address */
  my_mac_address.u32L = ieee_addr[7] | ieee_addr[6] << 8 | ieee_addr[5] << 16
      | ieee_addr[4] << 24;
  my_mac_address.u32H = ieee_addr[3] | ieee_addr[2] << 8 | ieee_addr[1] << 16
      | ieee_addr[0] << 24;
  if(ieee_addr == NULL) {
    vMMAC_GetMacAddress(&my_mac_address);
  }
  vMMAC_SetRxAddress(my_pan_id, my_short_address, &my_mac_address);
  int i;
  for(i=0; i<8; i++) {
    node_mac[i] = ieee_addr[i];
  }

  RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
/* Extract addresses from raw packet */
int tsch_packet_extract_addresses(uint8_t *buf, uint8_t len, linkaddr_t *source_address, linkaddr_t *dest_address);
static int
micromac_radio_is_packet_for_us(uint8_t* data, int len)
{
  linkaddr_t dest_address;
  uint8_t parsed = tsch_packet_extract_addresses(data, len, NULL, &dest_address);
  return parsed && (linkaddr_cmp(&dest_address, &linkaddr_node_addr)
               || linkaddr_cmp(&dest_address, &linkaddr_null));
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_read(void *buf, unsigned short bufsize)
{
  GET_LOCK();
  int len = 0;
  int packet_for_me = 0;
  len = input_frame_buffer->u8PayloadLength;
  if(len <= CHECKSUM_LEN) {
    PRINTF("micromac_radio: len %d <= CHECKSUM_LEN\n", len);
    RELEASE_LOCK();
    return 0;
  } else {
    len -= CHECKSUM_LEN;
    /* Check CRC */
#if CRC_SW
    uint16_t checksum = crc16_data(input_frame_buffer->uPayload.au8Byte, len, 0);
    radio_last_rx_crc =
        (uint16_t) (input_frame_buffer->uPayload.au8Byte[len + 1] << (uint16_t) 8)
            | input_frame_buffer->uPayload.au8Byte[len];
    radio_last_rx_crc_ok = (checksum == radio_last_rx_crc);
    if(!radio_last_rx_crc_ok) {
      RIMESTATS_ADD(badcrc);
    }
    if(radio_last_rx_crc_ok || MICROMAC_ACCEPT_FCS_ERROR)
#endif /* CRC_SW */
    {
      /* if interrupt_enabled then address check is done there */
      if(interrupt_enabled) {
        packet_for_me = 1;
      } else {
        /* if interrupt is disabled we need to read LQI */
        micromac_radio_last_lqi = u8MMAC_GetRxLqi(&micromac_radio_last_msq);
        radio_last_rssi = micromac_radio_last_msq;
        radio_last_correlation = micromac_radio_last_lqi;
        if(!address_decoding_enabled) {
          packet_for_me = 1;
        } else {
          packet_for_me = micromac_radio_is_packet_for_us(input_frame_buffer->uPayload.au8Byte, len);
        }
      }
      if(packet_for_me) {
        bufsize = MIN(len, bufsize);
        memcpy(buf, input_frame_buffer->uPayload.au8Byte, bufsize);
        RIMESTATS_ADD(llrx);
      } else { /* report an invalid packet */
        len = 0;
      }
    }
#if CRC_SW
    else {
      len = 0;
    }
    PRINTF("micromac_radio: reading %d bytes checksum %04x %04x, packet_for_me %d\n", len, checksum, radio_last_rx_crc, packet_for_me);
#else /* CRC_SW */
    PRINTF("micromac_radio: reading %d bytes, packet_for_me %d\n", len, packet_for_me);
#endif /* CRC_SW */
    /* Disable further read attempts */
    input_frame_buffer->u8PayloadLength = 0;
    RELEASE_LOCK();
  }
  pending = 0;

  return len;
}
/*---------------------------------------------------------------------------*/
void
micromac_radio_set_txpower(uint8_t power)
{
  GET_LOCK();
#if ENABLE_JPT
  vJPT_RadioSetPower(RF_POWER);
#endif /* ENABLE_JPT */
  RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_get_txpower(void)
{
  int power = 0;
  GET_LOCK();
#if ENABLE_JPT
  power = u8JPT_RadioGetPower();
#endif /* ENABLE_JPT */
  RELEASE_LOCK();
  return power;
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_detected_energy(void)
{
#if ENABLE_JPT
  uint32 u32Samples = 8;
  return u8JPT_EnergyDetect(channel, u32Samples);
#else
  return 0;
#endif /* ENABLE_JPT */
}
/*---------------------------------------------------------------------------*/
int
micromac_radio_receiving_packet(void)
{
  return bMMAC_RxDetected();
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_pending_packet(void)
{
  /* TODO check E_MMAC_INT_RX_HEADER too or only E_MMAC_INT_RX_COMPLETE ? */
  if(interrupt_enabled) {
    pending = (ringbufindex_peek_get(&input_ringbuf) != -1);
  } else {
    pending += u32MMAC_PollInterruptSource(
        E_MMAC_INT_RX_COMPLETE | E_MMAC_INT_RX_HEADER);
  }
  return pending;
}
/*---------------------------------------------------------------------------*/
static int
micromac_radio_cca(void)
{
  int cca = 0;
  /* If the radio is locked by an underlying thread (because we are
   being invoked through an interrupt), we pretend that the coast is
   clear (i.e., no packet is currently being transmitted by a
   neighbor). */
  if(locked) {
    return !cca;
  }

  GET_LOCK();
#if ENABLE_JPT
  cca = bJPT_CCA(channel, E_JPT_CCA_MODE_CARRIER_OR_ENERGY,
      cca_thershold);
#endif /* ENABLE_JPT */
  RELEASE_LOCK();
  return !cca;
}
/*---------------------------------------------------------------------------*/
void
micromac_radio_set_cca_threshold(int c)
{
  GET_LOCK();
  cca_thershold = c;
  RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
void
copy_from_linkaddress(tuAddr* tu_addr, linkaddr_t* addr)
{
  if(addr != NULL && tu_addr != NULL) {
#if(LINKADDR_SIZE==8)
    tu_addr->sExt.u32L=addr->u8[7] | addr->u8[6]<<8 | addr->u8[5]<<16 | addr->u8[4]<<24;
    tu_addr->sExt.u32H=addr->u8[3] | addr->u8[2]<<8 | addr->u8[1]<<16 | addr->u8[0]<<24;
#elif(LINKADDR_SIZE==2)
    tu_addr->u16Short = addr->u8[1] | addr->u8[0] << 8;
#endif
  }
}
/*---------------------------------------------------------------------------*/
void
copy_to_linkaddress(linkaddr_t* addr, tuAddr* tu_addr)
{
  if(addr != NULL && tu_addr != NULL) {
#if(LINKADDR_SIZE==8)
    addr->u8[7] = tu_addr->sExt.u32L;
    addr->u8[6] = tu_addr->sExt.u32L >> (uint32_t)8;
    addr->u8[5] = tu_addr->sExt.u32L >> (uint32_t)16;
    addr->u8[4] = tu_addr->sExt.u32L >> (uint32_t)24;
    addr->u8[3] = tu_addr->sExt.u32H;
    addr->u8[2] = tu_addr->sExt.u32H >> (uint32_t)8;
    addr->u8[1] = tu_addr->sExt.u32H >> (uint32_t)16;
    addr->u8[0] = tu_addr->sExt.u32H >> (uint32_t)24;
#elif(LINKADDR_SIZE==2)
    addr->u8[1] = tu_addr->u16Short;
    addr->u8[0] = tu_addr->u16Short >> (uint16_t) 8;
#endif
  }
}
/*---------------------------------------------------------------------------*/
void
micromac_radio_interrupt(uint32 mac_event)
{
  /* TODO another pass */
  uint32_t rx_state;
  uint8_t overflow = 0;
  int packet_for_me = 0;

  if(mac_event & E_MMAC_INT_TX_COMPLETE) {
    /* Transmission attempt has finished */
    tx_in_progress = 0;
  } else if(mac_event & (/*E_MMAC_INT_RX_HEADER |*/ E_MMAC_INT_RX_COMPLETE)) {
    rx_state = u32MMAC_GetRxErrors();
    /* if rx is successful */
    if(rx_state == 0) {
      /* Save SFD timestamp */
      last_packet_timestamp = micromac_radio_read_sfd_timer();

      if(interrupt_enabled && (mac_event & E_MMAC_INT_RX_COMPLETE)) {
        if(address_decoding_enabled && rx_frame_buffer->u8PayloadLength > CHECKSUM_LEN) {
          //check RX address
          packet_for_me = micromac_radio_is_packet_for_us(rx_frame_buffer->uPayload.au8Byte, rx_frame_buffer->u8PayloadLength - CHECKSUM_LEN);
        } else if(!address_decoding_enabled && rx_frame_buffer->u8PayloadLength > CHECKSUM_LEN) {
          packet_for_me = 1;
        }
        if(!packet_for_me) {
          /* prevent reading */
          rx_frame_buffer->u8PayloadLength = 0;
        } else {
          micromac_radio_last_lqi = u8MMAC_GetRxLqi(&micromac_radio_last_msq);
          radio_last_rssi = micromac_radio_last_msq;
          radio_last_correlation = micromac_radio_last_lqi;
          /* put the index to the received frame */
          ringbufindex_put(&input_ringbuf);
          //XXX disable process-poll to fill ringbuf and to stress-test it
#if !STRESS_TEST
          process_poll(&micromac_radio_process);
#endif
          /* get pointer to next input slot */
          int put_index = ringbufindex_peek_put(&input_ringbuf);
          /* is there space? */
          if(put_index != -1) {
            /* move rx_frame_buffer to next empty slot */
            rx_frame_buffer = &input_array[put_index];
          } else {
            overflow = 1;
            rx_frame_buffer = NULL;
            //XXX for stress test
#if STRESS_TEST
            process_poll(&micromac_radio_process);
#endif
          }
        }
      }
    } else { /* if rx is not successful */
      if(rx_state & E_MMAC_RXSTAT_ABORTED) {
        RIMESTATS_ADD(badsynch);
      } else if(rx_state & E_MMAC_RXSTAT_ERROR) {
        RIMESTATS_ADD(badcrc);
      } else if(rx_state & E_MMAC_RXSTAT_MALFORMED) {
        RIMESTATS_ADD(toolong);
      }
    }
    rx_in_progress = 0;
  }
  if(overflow) {
    off();
  } else if(always_on
      && (mac_event & (E_MMAC_INT_TX_COMPLETE | E_MMAC_INT_RX_COMPLETE))) {
    on();
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(micromac_radio_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("micromac_radio_process: started\n");

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    PRINTF("micromac_radio_process: polled\n");
    /* Pass received packets to upper layer */
    int16_t read_index;
    /* Loop on accessing (without removing) a pending input packet */
    while((read_index = ringbufindex_peek_get(&input_ringbuf)) != -1) {
      input_frame_buffer = &input_array[read_index];
      /* Put packet into packetbuf for input callback */
      packetbuf_clear();
      /* TODO PACKETBUF_ATTR_TIMESTAMP is 16bits while last_packet_timestamp is 32bits*/
#ifndef WITHOUT_ATTR_TIMESTAMP
      packetbuf_set_attr(PACKETBUF_ATTR_TIMESTAMP, last_packet_timestamp >> 16);
#endif /* WITHOUT_ATTR_TIMESTAMP */
      int len = micromac_radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
      /* XXX Another packet could have come; thus, the rssi value is wrongly matched to an older packet */
      packetbuf_set_attr(PACKETBUF_ATTR_RSSI, radio_last_rssi);
#ifndef WITHOUT_ATTR_LINK_QUALITY
      packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, radio_last_correlation);
#endif /* WITHOUT_ATTR_LINK_QUALITY */
      /* is packet valid? */
      if(len >0) {
        packetbuf_set_datalen(len);
        PRINTF("micromac_radio_process: calling receiver callback %u\n", p->u8PayloadLength);
        NETSTACK_RDC.input();
      }
      /* Remove packet from ringbuf */
      ringbufindex_get(&input_ringbuf);
      /* Disable further read attempts */
      input_frame_buffer->u8PayloadLength = 0;
    }

    /* Are we recovering from overflow? */
    if(rx_frame_buffer == NULL) {
      /* get pointer to next input slot */
      int put_index = ringbufindex_peek_put(&input_ringbuf);
      /* is there space? */
      if(put_index != -1) {
        /* move rx_frame_buffer to next empty slot */
        rx_frame_buffer = &input_array[put_index];
        /* do we need to turn radio on? */
        if(always_on || missed_radio_on_request) {
          missed_radio_on_request = 0;
          on();
        }
      } else {
        rx_frame_buffer = NULL;
      }
    }
  }
  PROCESS_END();
}

/* Turn on/off address decoding. XXX: rename to frame-decoding/parsing
 * Disabling address decoding would enable reception of
 * frames not compliant with the 802.15.4-2003 standard
 * with RX in PHY mode */
void
micromac_radio_address_decode(uint8_t enable)
{
  /* Turn on/off address decoding. */
  address_decoding_enabled = enable;
}

/* Enable or disable radio interrupts (both tx/rx) */
void
micromac_radio_set_interrupt_enable(uint8_t e)
{
  GET_LOCK();

  interrupt_enabled = e;
  if(!interrupt_enabled) {
    /* Disable interrupts */
    vMMAC_EnableInterrupts(NULL);
    vMMAC_ConfigureInterruptSources(0);
  } else {
    /* Initialize and enable interrupts */
    vMMAC_ConfigureInterruptSources(
        E_MMAC_INT_RX_HEADER | E_MMAC_INT_RX_COMPLETE | E_MMAC_INT_TX_COMPLETE);
    vMMAC_EnableInterrupts(&micromac_radio_interrupt);
  }
  RELEASE_LOCK();
}

/* Enable or disable radio always on */
void
micromac_radio_set_always_on(uint8_t e)
{
  GET_LOCK();
  always_on = e;
  RELEASE_LOCK();
}
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  int i, v;

  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    *value = rx_in_progress || tx_in_progress ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = micromac_radio_get_channel();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    v = micromac_radio_get_txpower();
    *value = OUTPUT_POWER_MIN;
    /* Find the actual estimated output power in conversion table */
    for(i = 0; i < OUTPUT_NUM; i++) {
      if(v >= output_power[i].config) {
        *value = output_power[i].power;
        break;
      }
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    /* Return the RSSI value in dBm */
    *value = radio_last_rssi;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MIN:
    *value = 11;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MAX:
    *value = 26;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MIN:
    *value = OUTPUT_POWER_MIN;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MAX:
    *value = OUTPUT_POWER_MAX;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}

static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  int i;

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      micromac_radio_on();
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_OFF) {
      micromac_radio_off();
      return RADIO_RESULT_OK;
    }
    return RADIO_RESULT_INVALID_VALUE;
  case RADIO_PARAM_CHANNEL:
    if(value < 11 || value > 26) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    micromac_radio_set_channel(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    if(value < OUTPUT_POWER_MIN || value > OUTPUT_POWER_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    /* Find the closest higher PA_LEVEL for the desired output power */
    for(i = 1; i < OUTPUT_NUM; i++) {
      if(value > output_power[i].power) {
        break;
      }
    }
    micromac_radio_set_txpower(output_power[i - 1].config);
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}

static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}

static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver micromac_radio_driver = {
  micromac_radio_init,
  micromac_radio_prepare,
  micromac_radio_transmit,
  micromac_radio_send,
  micromac_radio_read,
  micromac_radio_cca,
  micromac_radio_receiving_packet,
  micromac_radio_pending_packet,
  micromac_radio_on,
  micromac_radio_off,
  get_value,
  set_value,
  get_object,
  set_object
};
