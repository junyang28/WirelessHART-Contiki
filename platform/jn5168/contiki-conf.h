/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

//#include <inttypes.h>
#include <jendefs.h>

#undef putchar

#define RADIO_TEST_MODE_HIGH_PWR 1
#define RADIO_TEST_MODE_ADVANCED 2
#define RADIO_TEST_MODE_DISABLED 0

#ifndef RADIO_TEST_MODE
#define RADIO_TEST_MODE  RADIO_TEST_MODE_DISABLED
#endif /* RADIO_TEST_MODE */

/* We disabled MAC parsing in HW */
#define RADIO_PARSE_MAC_HW 0

/* UART baud rates */
#define UART_RATE_4800          0
#define UART_RATE_9600          1
#define UART_RATE_19200         2
#define UART_RATE_38400         3
#define UART_RATE_76800         4
#define UART_RATE_115200        5
#define UART_RATE_230400        6
#define UART_RATE_460800        7
#define UART_RATE_500000        8
#define UART_RATE_576000        9
#define UART_RATE_921600        10
#define UART_RATE_1000000       11

#define PLATFORM_HAS_LEDS    1
#define PLATFORM_HAS_BUTTON  0 /* sensor driver not implemented */
#define PLATFORM_HAS_LIGHT   0 /* sensor driver not implemented */
#define PLATFORM_HAS_BATTERY 0 /* sensor driver not implemented */
#define PLATFORM_HAS_SHT11   0 /* sensor driver not implemented */
#define PLATFORM_HAS_RADIO   1

/* CPU target speed in Hz
 * RTIMER and peripherals clock is F_CPU/2 */
#define F_CPU 32000000UL

/* LED ports */
/*
#define LEDS_PxDIR P5DIR
#define LEDS_PxOUT P5OUT
#define LEDS_CONF_RED    0x10
#define LEDS_CONF_GREEN  0x20
#define LEDS_CONF_YELLOW 0x40
#define JENNIC_CONF_BUTTON_PIN (IRQ_DIO9|IRQ_DIO10)
*/

#define CC_CONF_REGISTER_ARGS          1
#define CC_CONF_FUNCTION_POINTER_ARGS  1
#define CC_CONF_FASTCALL
#define CC_CONF_VA_ARGS                1
#define CC_CONF_INLINE                 inline

//#ifndef EEPROM_CONF_SIZE
//#define EEPROM_CONF_SIZE				1024
//#endif

#define CCIF
#define CLIF

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#ifndef uint8_t
typedef unsigned char   uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
typedef   signed char    int8_t;
typedef          short  int16_t;
typedef          long   int32_t;
typedef unsigned long long uint64_t;
typedef long long          int64_t;
#endif
#endif /* !HAVE_STDINT_H */

/* These names are deprecated, use C99 names. */
typedef uint8_t   u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;
typedef  int32_t s32_t;
typedef  int64_t s64_t;

#if !true
#define true (1)
#define false (0)
#endif

/* Types for clocks and uip_stats */
typedef uint16_t uip_stats_t;
typedef uint32_t clock_time_t;

/* Core rtimer.h defaults to 16 bit timer unless RTIMER_CLOCK_LT is defined */
typedef uint32_t rtimer_clock_t;
#define RTIMER_CLOCK_LT(a,b)     ((int32_t)((a)-(b)) < 0)
/* 10ms timer tick */
#define CLOCK_CONF_SECOND 100

/* Shall we calibrate the DCO periodically? */
#define DCOSYNCH_CONF_ENABLED 1

/* How often shall we attempt to calibrate DCO?
 * PS: It should be calibrated upon temperature changes,
 * but the naive approach of periodic calibration is fine too */
#ifndef DCOSYNCH_PERIOD
#define DCOSYNCH_PERIOD (5*60)
#endif /* VCO_CALIBRATION_INTERVAL */

/* Disable UART HW flow control */
#ifndef UART_HW_FLOW_CTRL
#define UART_HW_FLOW_CTRL 0
#endif /* UART_HW_FLOW_CTRL */

/* Disable UART SW flow control */
#ifndef UART_XONXOFF_FLOW_CTRL
#define UART_XONXOFF_FLOW_CTRL 0
#endif /* UART_XONXOFF_FLOW_CTRL */

#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE UART_RATE_1000000
#endif /* UART_BAUD_RATE */

/* Set this to zero only if we are using SLIP */
#ifndef SLIP_BRIDGE_CONF_NO_PUTCHAR
#define SLIP_BRIDGE_CONF_NO_PUTCHAR 1
#endif /* SLIP_BRIDGE_CONF_NO_PUTCHAR */

#define ENABLE_ADVANCED_BAUD_SELECTION (UART_BAUD_RATE > UART_RATE_115200)

#ifndef USE_SLIP_UART1
#define USE_SLIP_UART1 0
#endif /* USE_SLIP_UART1 */

#define RIMESTATS_CONF_ENABLED 0

#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     nullmac_driver
#endif /* NETSTACK_CONF_MAC */

#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nullrdc_driver
#endif /* NETSTACK_CONF_RDC */

#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */

#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   micromac_radio_driver
#endif /* NETSTACK_CONF_RADIO */

#ifndef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154
#endif /* NETSTACK_CONF_FRAMER */

#ifndef MMAC_CONF_AUTOACK
#define MMAC_CONF_AUTOACK              1
#endif /* MMAC_CONF_AUTOACK */

/* Specify whether the RDC layer should enable
   per-packet power profiling. */
#define CONTIKIMAC_CONF_COMPOWER         1
#define XMAC_CONF_COMPOWER               1
#define CXMAC_CONF_COMPOWER              1

#define PACKETBUF_CONF_ATTRS_INLINE 1

#define CONTIKIMAC_CONF_BROADCAST_RATE_LIMIT 0

#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif /* NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE */

#ifndef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID       0xABCD
#endif /* IEEE802154_CONF_PANID */

//#define SHELL_VARS_CONF_RAM_BEGIN 0x1100
//#define SHELL_VARS_CONF_RAM_END 0x2000

#define PROFILE_CONF_ON 0
#ifndef ENERGEST_CONF_ON
#define ENERGEST_CONF_ON 1
#endif /* ENERGEST_CONF_ON */

//#define ELFLOADER_CONF_TEXT_IN_ROM 0
//#ifndef ELFLOADER_CONF_DATAMEMORY_SIZE
//#define ELFLOADER_CONF_DATAMEMORY_SIZE 0x400
//#endif /* ELFLOADER_CONF_DATAMEMORY_SIZE */
//#ifndef ELFLOADER_CONF_TEXTMEMORY_SIZE
//#define ELFLOADER_CONF_TEXTMEMORY_SIZE 0x800
//#endif /* ELFLOADER_CONF_TEXTMEMORY_SIZE */


#define AODV_COMPLIANCE
#define AODV_NUM_RT_ENTRIES 32

#define WITH_ASCII 1

#define PROCESS_CONF_NUMEVENTS 8
#define PROCESS_CONF_STATS 1
/*#define PROCESS_CONF_FASTPOLL    4*/

#ifdef NETSTACK_CONF_WITH_IPV6
#define UIP_CONF_BROADCAST 1
/* Network setup for IPv6 */
#define NETSTACK_CONF_NETWORK sicslowpan_driver

/* Specify a minimum packet size for 6lowpan compression to be
   enabled. This is needed for ContikiMAC, which needs packets to be
   larger than a specified size, if no ContikiMAC header should be
   used. */
#define SICSLOWPAN_CONF_COMPRESSION_THRESHOLD 63
#define CONTIKIMAC_CONF_WITH_CONTIKIMAC_HEADER 0

#define CXMAC_CONF_ANNOUNCEMENTS         0
#define XMAC_CONF_ANNOUNCEMENTS          0

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM                8
#endif

#define LINKADDR_CONF_SIZE              8

#define UIP_CONF_LL_802154              1
#define UIP_CONF_LLH_LEN                0

#define UIP_CONF_ROUTER                 1
#ifndef UIP_CONF_IPV6_RPL
#define UIP_CONF_IPV6_RPL               1
#endif /* UIP_CONF_IPV6_RPL */

/* configure number of neighbors and routes */
#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     20
#endif /* NBR_TABLE_CONF_MAX_NEIGHBORS */
#ifndef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES   20
#endif /* UIP_CONF_MAX_ROUTES */

#define UIP_CONF_ND6_SEND_RA		        0
#define UIP_CONF_ND6_REACHABLE_TIME     600000
#define UIP_CONF_ND6_RETRANS_TIMER      10000

#define UIP_CONF_IPV6                   1
#ifndef UIP_CONF_IPV6_QUEUE_PKT
#define UIP_CONF_IPV6_QUEUE_PKT         0
#endif /* UIP_CONF_IPV6_QUEUE_PKT */
#define UIP_CONF_IPV6_CHECKS            1
#define UIP_CONF_IPV6_REASSEMBLY        0
#define UIP_CONF_NETIF_MAX_ADDRESSES    3
#define UIP_CONF_ND6_MAX_PREFIXES       3
#define UIP_CONF_ND6_MAX_DEFROUTERS     2
#define UIP_CONF_IP_FORWARD             0
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE		        240
#endif

#define SICSLOWPAN_CONF_COMPRESSION_IPV6        0
#define SICSLOWPAN_CONF_COMPRESSION_HC1         1
#define SICSLOWPAN_CONF_COMPRESSION_HC01        2
#define SICSLOWPAN_CONF_COMPRESSION             SICSLOWPAN_COMPRESSION_HC06
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG                    1
#define SICSLOWPAN_CONF_MAXAGE                  8
#endif /* SICSLOWPAN_CONF_FRAG */
#define SICSLOWPAN_CONF_CONVENTIONAL_MAC	1
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS       2
#ifndef SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS
#define SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS   5
#endif /* SICSLOWPAN_CONF_MAX_MAC_TRANSMISSIONS */
#define UIP_CONF_ICMP_DEST_UNREACH 1

#define UIP_CONF_DHCP_LIGHT
#ifndef  UIP_CONF_RECEIVE_WINDOW
#define UIP_CONF_RECEIVE_WINDOW  48
#endif
#ifndef  UIP_CONF_TCP_MSS
#define UIP_CONF_TCP_MSS         48
#endif
#define UIP_CONF_MAX_CONNECTIONS 4
#define UIP_CONF_MAX_LISTENPORTS 8
#define UIP_CONF_UDP_CONNS       12
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
// TODO implement architecture optimized functions
//#define UIP_ARCH_IPCHKSUM
#define UIP_ARCH_CHKSUM 				 0
#define UIP_ARCH_ADD32					 0
#define UIP_CONF_UDP             1
#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0
#define LOG_CONF_ENABLED         0

#define UIP_CONF_TCP_SPLIT       0

#define UIP_CONF_BYTE_ORDER      UIP_BIG_ENDIAN
#define UIP_CONF_TCP       			 0
#define UIP_CONF_LOGGING         0
#else /* NETSTACK_CONF_WITH_IPV6 */

/* Network setup for non-IPv6 (rime). */

#define NETSTACK_CONF_NETWORK rime_driver

#define COLLECT_CONF_ANNOUNCEMENTS       1
#define CXMAC_CONF_ANNOUNCEMENTS         0
#define XMAC_CONF_ANNOUNCEMENTS          0
#define CONTIKIMAC_CONF_ANNOUNCEMENTS    0

#ifndef COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS
#define COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS     32
#endif /* COLLECT_NEIGHBOR_CONF_MAX_COLLECT_NEIGHBORS */

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM                16
#endif /* QUEUEBUF_CONF_NUM */

#ifndef TIMESYNCH_CONF_ENABLED
#define TIMESYNCH_CONF_ENABLED           0
#endif /* TIMESYNCH_CONF_ENABLED */

#define UIP_CONF_IP_FORWARD      1
#define UIP_CONF_BUFFER_SIZE     108
#define LINKADDR_CONF_SIZE       2
#endif /* NETSTACK_CONF_WITH_IPV6 */

#ifndef AES_128_CONF
/* TODO #define AES_128_CONF .... */
#endif /* AES_128_CONF */

/* include the project config */
/* PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif /* PROJECT_CONF_H */

#endif /* CONTIKI_CONF_H_ */
