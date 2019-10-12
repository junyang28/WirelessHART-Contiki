/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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

/**
 * \file
 *         CC2420 driver header file
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CC2420_H_
#define CC2420_H_

#include "contiki.h"
#include "dev/spi.h"
#include "dev/radio.h"
#include "cc2420_const.h"
#include "lib/aes-128.h"

int cc2420_init(void);

#define CC2420_MAX_PACKET_LEN      127

#define ACK_LEN 3

int cc2420_set_channel(int channel);
int cc2420_get_channel(void);

void cc2420_set_pan_addr(unsigned pan,
                                unsigned addr,
                                const uint8_t *ieee_addr);

extern signed char cc2420_last_rssi;
extern uint8_t cc2420_last_correlation;

int cc2420_rssi(void);

extern const struct radio_driver cc2420_driver;

/**
 * \param power Between 1 and 31.
 */
void cc2420_set_txpower(uint8_t power);
int cc2420_get_txpower(void);
#define CC2420_TXPOWER_MAX  31
#define CC2420_TXPOWER_MIN   0

/**
 * Interrupt function, called from the simple-cc2420-arch driver.
 *
 */
int cc2420_interrupt(void);

/* XXX hack: these will be made as Chameleon packet attributes */
extern rtimer_clock_t cc2420_time_of_arrival,
  cc2420_time_of_departure;
extern int cc2420_authority_level_of_sender;

int cc2420_on(void);
int cc2420_off(void);

void cc2420_set_cca_threshold(int value);

extern const struct aes_128_driver cc2420_aes_128_driver;

/* Configures timer B to capture SFD edge (start, end, both),
 * and sets the link start time for calculating synchronization in ACK */
void cc2420_sfd_sync(uint8_t capture_start_sfd, uint8_t capture_end_sfd);

/* Read the timer value when the last SFD edge was captured,
 * this depends on SFD timer configuration */
uint16_t cc2420_read_sfd_timer(void);

/* Turn on/off address decoding.
 * Disabling address decoding would enable reception of
 * frames not compliant with the 802.15.4-2003 standard */
void cc2420_address_decode(uint8_t enable);

/* Enable or disable radio interrupts (both FIFOP and SFD timer capture) */
void cc2420_set_interrupt_enable(uint8_t e);

/* Get radio interrupt enable status */
uint8_t cc2420_get_interrupt_enable(void);

/************************************************************************/
/* Generic names for special functions */
/************************************************************************/

#define NETSTACK_RADIO_address_decode(E)        cc2420_address_decode((E))
#define NETSTACK_RADIO_set_interrupt_enable(E)  cc2420_set_interrupt_enable((E))
#define NETSTACK_RADIO_sfd_sync(S,E)            cc2420_sfd_sync((S),(E))
#define NETSTACK_RADIO_read_sfd_timer()         cc2420_read_sfd_timer()
#define NETSTACK_RADIO_set_channel(C)           cc2420_set_channel((C))
#define NETSTACK_RADIO_get_channel()            cc2420_get_channel()
#define NETSTACK_RADIO_radio_raw_rx_on()        cc2420_on();
#define NETSTACK_RADIO_set_txpower(X)           cc2420_set_txpower(X)
#define NETSTACK_RADIO_set_cca_threshold(X)     cc2420_set_cca_threshold(X)

/* Read status of the CC2420 */
#define CC2420_GET_STATUS(s)                       \
  do {                                          \
    CC2420_SPI_ENABLE();                        \
    SPI_WRITE(CC2420_SNOP);                     \
    s = SPI_RXBUF;                              \
    CC2420_SPI_DISABLE();                       \
  } while (0)

#endif /* CC2420_H_ */
