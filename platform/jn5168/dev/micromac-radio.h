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
 *         MICROMAC_RADIO driver header file
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 */

#ifndef MICROMAC_RADIO_H_
#define MICROMAC_RADIO_H_

#include "contiki.h"
#include "dev/radio.h"
#include "linkaddr.h"
#include "net/mac/frame802154.h"
#include <MMAC.h>

/* RTIMER 16M = 256 * 62500(RADIO)  == 2^8 * 62500 */
/* TODO check if converting signed values */
#define RADIO_TO_RTIMER(X) 											((rtimer_clock_t)( (X) << (int32_t)8L ))
#define RTIMER_TO_RADIO(X) 											((uint32_t)( (X) >> (int32_t)8L ))
#define MICROMAC_RADIO_MAX_PACKET_LEN      			(127)
#define ACK_LEN (3)
#define MICROMAC_HEADER_LEN (28)

#define MAX_PACKET_DURATION RADIO_TO_RTIMER(2*(MICROMAC_RADIO_MAX_PACKET_LEN+1))

#define MICROMAC_RADIO_TXPOWER_MAX  3
#define MICROMAC_RADIO_TXPOWER_MIN  0

#undef  RADIO_TXPOWER_MAX
#define RADIO_TXPOWER_MAX  MICROMAC_RADIO_TXPOWER_MAX
#undef  RADIO_TXPOWER_MIN
#define RADIO_TXPOWER_MIN  MICROMAC_RADIO_TXPOWER_MIN

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time)));   \
  } while(0)

void copy_from_linkaddress(tuAddr*, linkaddr_t* );
void copy_to_linkaddress(linkaddr_t* addr, tuAddr* tu_addr);

int micromac_radio_init(void);

int micromac_radio_set_channel(int channel);
int micromac_radio_get_channel(void);

void micromac_radio_set_pan_addr(unsigned pan,
                                unsigned addr,
                                const uint8_t *ieee_addr);

extern const struct radio_driver micromac_radio_driver;

/**
 * \param power Between 0 and 3.
 */
void micromac_radio_set_txpower(uint8_t power);
int micromac_radio_get_txpower(void);

int micromac_radio_on(void);
int micromac_radio_off(void);

void micromac_radio_set_cca_threshold(int value);

void micromac_radio_sfd_sync(void);
rtimer_clock_t micromac_radio_read_sfd_timer(void);
void micromac_radio_interrupt(uint32 mac_event);

/* Turn on/off address decoding.
 * Disabling address decoding would enable reception of
 * frames not compliant with the 802.15.4-2003 standard
 * with RX in PHY mode */
void micromac_radio_address_decode(uint8_t enable);

/* Enable or disable radio interrupts (both tx/rx) */
void micromac_radio_set_interrupt_enable(uint8_t e);

int micromac_radio_raw_rx_on(void);

/* Enable or disable radio always on */
void micromac_radio_set_always_on(uint8_t e);

/************************************************************************/
/* Generic names for special functions */
/************************************************************************/
#define NETSTACK_RADIO_radio_raw_rx_on()        micromac_radio_raw_rx_on();
#define NETSTACK_RADIO_tx_duration(X)           RADIO_TO_RTIMER((X)+1)
#define NETSTACK_RADIO_get_time()               u32MMAC_GetTime()
#define NETSTACK_RADIO_address_decode(E)        micromac_radio_address_decode((E))
#define NETSTACK_RADIO_set_interrupt_enable(E)  micromac_radio_set_interrupt_enable((E))
#define NETSTACK_RADIO_sfd_sync(S,E)            micromac_radio_sfd_sync()
#define NETSTACK_RADIO_read_sfd_timer()         micromac_radio_read_sfd_timer()
#define NETSTACK_RADIO_set_channel(C)           micromac_radio_set_channel((C))
#define NETSTACK_RADIO_get_channel              micromac_radio_get_channel
#define NETSTACK_RADIO_set_txpower(X)           micromac_radio_set_txpower(X)
#define NETSTACK_RADIO_set_cca_threshold(X)     micromac_radio_set_cca_threshold(X)

#endif /* MICROMAC_RADIO_H_ */
