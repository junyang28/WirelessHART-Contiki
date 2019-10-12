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
 */
/**
 * \file
 *         Orchestra header file
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __ORCHESTRA_H__
#define __ORCHESTRA_H__

#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"

#if ORCHESTRA_CONFIG == ORCHESTRA_MINIMAL_SCHEDULE

#define ORCHESTRA_WITH_COMMON_SHARED              1
#define ORCHESTRA_COMMON_SHARED_PERIOD            TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
#define ORCHESTRA_COMMON_SHARED_TYPE              LINK_TYPE_ADVERTISING

#elif ORCHESTRA_CONFIG == ORCHESTRA_RECEIVER_BASED

#define ORCHESTRA_WITH_EBSF                       1
#define ORCHESTRA_EBSF_PERIOD                     397
#define ORCHESTRA_WITH_COMMON_SHARED              1
#define ORCHESTRA_COMMON_SHARED_TYPE              LINK_TYPE_NORMAL
#define ORCHESTRA_COMMON_SHARED_PERIOD            31
#define ORCHESTRA_WITH_RBUNICAST                  1
#define ORCHESTRA_RBUNICAST_PERIOD                ORCHESTRA_UNICAST_PERIOD

#elif ORCHESTRA_CONFIG == ORCHESTRA_SENDER_BASED

#define ORCHESTRA_WITH_EBSF                       1
#define ORCHESTRA_EBSF_PERIOD                     397
#define ORCHESTRA_WITH_COMMON_SHARED              1
#define ORCHESTRA_COMMON_SHARED_TYPE              LINK_TYPE_NORMAL
#define ORCHESTRA_COMMON_SHARED_PERIOD            31
#define ORCHESTRA_WITH_SBUNICAST                  1
#define ORCHESTRA_SBUNICAST_PERIOD                ORCHESTRA_UNICAST_PERIOD
#ifdef ORCHESTRA_UNICAST_PERIOD2
#define ORCHESTRA_SBUNICAST_PERIOD2               ORCHESTRA_UNICAST_PERIOD2
#endif
#ifdef ORCHESTRA_UNICAST_PERIOD2
#define ORCHESTRA_SBUNICAST_SHARED               0
#else
#define ORCHESTRA_SBUNICAST_SHARED               (ORCHESTRA_UNICAST_PERIOD < MAX_NODES)
#endif

#endif

void orchestra_init();
void orchestra_callback_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new);
void orchestra_callback_joining_network();
int orchestra_callback_do_nack(struct tsch_link *link, linkaddr_t *src, linkaddr_t *dst);

#endif /* __ORCHESTRA_H__ */
