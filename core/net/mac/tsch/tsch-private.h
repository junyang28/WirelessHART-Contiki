/*
 * Copyright (c) 2014, Swedish Institute of Computer Science.
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
 *         Private TSCH definitions
 *         (meant for use by TSCH implementation files only)
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 *         Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __TSCH_PRIVATE_H__
#define __TSCH_PRIVATE_H__

#include "contiki.h"
#include "net/linkaddr.h"

/* Max time before sending a unicast keep-alive message to the time source */
#ifdef TSCH_CONF_KEEPALIVE_TIMEOUT
#define TSCH_KEEPALIVE_TIMEOUT TSCH_CONF_KEEPALIVE_TIMEOUT
#else
/* Time to desynch assuming a drift of 40 PPM (80 PPM between two nodes) and guard time of +/-1ms: 12.5s. */
#define TSCH_KEEPALIVE_TIMEOUT (12 * CLOCK_SECOND)
#endif

#ifdef TSCH_CONF_DESYNC_THRESHOLD
#define TSCH_DESYNC_THRESHOLD TSCH_CONF_DESYNC_THRESHOLD
#else
#define TSCH_DESYNC_THRESHOLD (4 * TSCH_KEEPALIVE_TIMEOUT)
#endif

/* Min period between two consecutive EBs */
#ifdef TSCH_CONF_MIN_EB_PERIOD
#define TSCH_MIN_EB_PERIOD TSCH_CONF_MIN_EB_PERIOD
#else
#define TSCH_MIN_EB_PERIOD (4 * CLOCK_SECOND)
#endif

/* Max period between two consecutive EBs */
#ifdef TSCH_CONF_MAX_EB_PERIOD
#define TSCH_MAX_EB_PERIOD TSCH_CONF_MAX_EB_PERIOD
#else
#define TSCH_MAX_EB_PERIOD (60 * CLOCK_SECOND)
#endif

/* Max acceptable join priority */
#ifdef TSCH_CONF_MAX_JOIN_PRIORITY
#define TSCH_MAX_JOIN_PRIORITY TSCH_CONF_MAX_JOIN_PRIORITY
#else
#define TSCH_MAX_JOIN_PRIORITY 16
#endif

/* Rx guard time (TsLongGT) */
#ifdef TSCH_CONF_GUARD_TIME
#define TSCH_GUARD_TIME TSCH_CONF_GUARD_TIME
#else
#define TSCH_GUARD_TIME 1000
#endif

/* Max number of links */
#ifdef TSCH_CONF_MAX_LINKS
#define TSCH_MAX_LINKS TSCH_CONF_MAX_LINKS
#else
#define TSCH_MAX_LINKS 32
#endif

/* TSCH MAC parameters */
#define MAC_MIN_BE 0
#define MAC_MAX_FRAME_RETRIES 8
#define MAC_MAX_BE 4

/* TSCH packet len */
#define TSCH_MAX_PACKET_LEN 127
#define TSCH_BASE_ACK_LEN 3
#define TSCH_SYNC_IE_LEN 4
#define TSCH_EACK_DEST_LEN (LINKADDR_SIZE+2) /* Dest PAN ID + dest MAC address */
/* Send enhanced ACK with Information Elements or Std ACK? */
#define TSCH_PACKET_WITH_SYNC_IE 1
#ifdef TSCH_CONF_PACKET_DEST_ADDR_IN_ACK
#define TSCH_PACKET_DEST_ADDR_IN_ACK TSCH_CONF_PACKET_DEST_ADDR_IN_ACK
#else
#define TSCH_PACKET_DEST_ADDR_IN_ACK 0
#endif
#define TSCH_ACK_LEN (TSCH_BASE_ACK_LEN \
                      + TSCH_PACKET_WITH_SYNC_IE*TSCH_SYNC_IE_LEN \
                      + TSCH_PACKET_DEST_ADDR_IN_ACK*TSCH_EACK_DEST_LEN)

/* Calculate packet tx/rc duration based on sent packet len assuming 802.15.4 250kbps data rate
 * PHY packet length: payload_len + CHECKSUM_LEN(2) + PHY_LEN_FIELD(1) */
#define TSCH_PACKET_DURATION(payload_len) (RADIO_TO_RTIMER(((payload_len) + 2 + 1) * 2))

/* Max time to wait for end of packet reception. Add a 350us margin. */
/* Check the following on JN5168. We used 4300us and 500us before */
#define TSCH_DATA_MAX_DURATION ((unsigned)(TSCH_PACKET_DURATION(TSCH_MAX_PACKET_LEN) + US_TO_RTIMERTICKS(350)))
#define TSCH_ACK_MAX_DURATION  ((unsigned)(TSCH_PACKET_DURATION(TSCH_ACK_LEN) + US_TO_RTIMERTICKS(350)))

/* Timeslot timing */
#define TsCCAOffset         ((unsigned)US_TO_RTIMERTICKS(1800))
#define TsCCA               ((unsigned)US_TO_RTIMERTICKS(128))

#define TsTxOffset          ((unsigned)US_TO_RTIMERTICKS(4000))
#define TsTxAckDelay        ((unsigned)US_TO_RTIMERTICKS(4000))
#define TsLongGT            ((unsigned)US_TO_RTIMERTICKS(TSCH_GUARD_TIME))
#define TsShortGT           ((unsigned)US_TO_RTIMERTICKS(400))
#define TsSlotDuration      ((unsigned)US_TO_RTIMERTICKS(15000))

/* The ASN is an absolute slot number over 5 bytes. */
struct asn_t {
  uint32_t ls4b; /* least significant 4 bytes */
  uint8_t  ms1b; /* most significant 1 byte */
};

/* For quick modulo operation on ASN */
struct asn_divisor_t {
  uint16_t val; /* Divisor value */
  uint16_t asn_ms1b_remainder; /* Remainder of the operation 0x100000000 / val */
};

/* Initialize ASN */
#define ASN_INIT(asn, ms1b_, ls4b_) do { \
  (asn).ms1b = (ms1b_); \
  (asn).ls4b = (ls4b_); \
} while(0);

/* Increment an ASN by inc (32 bits) */
#define ASN_INC(asn, inc) do { \
  uint32_t new_ls4b = (asn).ls4b + (inc); \
  if(new_ls4b < (asn).ls4b) (asn).ms1b++; \
  (asn).ls4b = new_ls4b; \
} while(0);

/* Decrement an ASN by inc (32 bits) */
#define ASN_DEC(asn, dec) do { \
  uint32_t new_ls4b = (asn).ls4b - (dec); \
  if(new_ls4b > (asn).ls4b) (asn).ms1b--; \
  (asn).ls4b = new_ls4b; \
} while(0);

/* Returns the 32-bit diff between asn1 and asn2 */
#define ASN_DIFF(asn1, asn2) \
    ((asn1).ls4b - (asn2).ls4b)

/* Initialize a struct asn_divisor_t */
#define ASN_DIVISOR_INIT(div, val_) do { \
  (div).val = (val_); \
  (div).asn_ms1b_remainder = ((0xffffffff % (val_)) + 1) % (val_); \
} while(0);

/* Returns the result (16 bits) of a modulo operation on ASN,
 * with divisor being a struct asn_divisor_t */
#define ASN_MOD(asn, div) \
  ((uint16_t)((asn).ls4b % (div).val) \
  +(uint16_t)((asn).ms1b * (div).asn_ms1b_remainder % (div).val)) \
  % (div).val

/* 802.15.4 broadcast MAC address */
extern const linkaddr_t tsch_broadcast_address;
/* The address we use to identify EB queue */
extern const linkaddr_t tsch_eb_address;

/* The current Absolute Slot Number (ASN) */
extern struct asn_t current_asn;
extern uint8_t tsch_join_priority;
extern struct tsch_link *current_link;

/* Are we associated to a TSCH network? */
extern int associated;

/* Is TSCH locked? */
int tsch_is_locked();
/* Lock TSCH (no link operation) */
int tsch_get_lock();
/* Release TSCH lock */
void tsch_release_lock();

/* Returns a 802.15.4 channel from an ASN and channel offset */
uint8_t tsch_calculate_channel(struct asn_t *asn, uint8_t channel_offset);
/* The the period at which EBs are sent */
void tsch_set_eb_period(uint32_t period);
/* Brief dump of the TSCH state */
void tsch_dump_status();

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */

/* TODO Update slot-timing terminology based on the standard */
/* TODO: move platform-specific code away from core */
#if CONTIKI_TARGET_JN5168

#define RSSI_CORRECTION_CONSTANT 0

#define US_TO_RTIMERTICKS(D) ((int64_t)(D) * 16L)
/* TODO check if shift operation preserves sign bit on jennic */
#define RTIMERTICKS_TO_US(T)   ((int64_t)(T) >> 4L)

#define RTIMER_MIN_DELAY (70 * 16UL) /* 70us */

/* radio speed related */
#define delayTx           ((unsigned)US_TO_RTIMERTICKS(350)) /* measured ~350us: 100uSec + between GO signal and SFD: radio fixed delay + 4Bytes preamble + 1B SFD -- 1Byte time is 31.25us */
#define delayRx           ((unsigned)US_TO_RTIMERTICKS(248)) /* measured 248us: between GO signal and start listening */

/* Convert rtimer ticks to clock and vice versa */
#define TSCH_CLOCK_TO_TICKS(c) ((c)*(RTIMER_SECOND/CLOCK_SECOND)) /* beware of computation overflow! */
#define TSCH_CLOCK_TO_SLOTS(c) (TSCH_CLOCK_TO_TICKS(c)/TsSlotDuration)

#else /* CONTIKI_TARGET_JN5168 */

/* For target Sky and Z1 */
#define RSSI_CORRECTION_CONSTANT (-45)

/* Do the math in 32bits to save precision.
 * Round to nearest integer rather than truncate. */
#define US_TO_RTIMERTICKS(US)   (((US)>=0) ? \
                               ((((int32_t)(US)*32768L)+500000) / 1000000L) : \
                               ((((int32_t)(US)*32768L)-500000) / 1000000L))
#define RTIMERTICKS_TO_US(T)   (((T)>=0) ? \
                               ((((int32_t)(T)*1000000L)+16384) / 32768L) : \
                               ((((int32_t)(T)*1000000L)-16384) / 32768L))

/* TODO check the following */
#define RADIO_TO_RTIMER(X)  ((rtimer_clock_t)((int32_t)((int32_t)(X) * 524L) / (int32_t)1000L))

#define RTIMER_MIN_DELAY 5

/* radio speed related */
  /* ~327us+129preample */
#define delayTx 15         /* between GO signal and SFD: radio fixed delay + 4Bytes preample + 1B SFD -- 1Byte time is 32us */
  /* ~50us delay + 129preample + ?? */
#define delayRx 6         /* between GO signal and start listening */
  /* radio watchdog */

/* Convert rtimer ticks to clock and vice versa */
#define TSCH_CLOCK_TO_TICKS(c) (((c)*RTIMER_SECOND)/CLOCK_SECOND)
#define TSCH_CLOCK_TO_SLOTS(c) (TSCH_CLOCK_TO_TICKS(c)/TsSlotDuration)

#endif /* CONTIKI_TARGET_JN5168 */

#endif /* __TSCH_PRIVATE_H__ */
