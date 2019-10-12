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
 *         Example file using RPL for a any-to-any routing: the root and all
 *         nodes with an even node-id send a ping periodically to another node
 *         (which also must be root or have even node-id). Upon receiving a ping,
 *         nodes answer with a poing.
 *         Can be deployed in the Indriya or Twist testbeds.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki-conf.h"
#include "net/netstack.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "lib/random.h"
#include "deployment.h"
#include "simple-udp.h"
#include "net/ip/uip-debug.h"
#include "lib/ringbufindex.h"
#include <stdio.h>

#define SEND_INTERVAL   (60 * CLOCK_SECOND)
#define UDP_PORT 1234

#define COORDINATOR_ID ROOT_ID

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_sender_process, "Link Probing Application");
AUTOSTART_PROCESSES(&broadcast_sender_process);
PROCESS(pending_events_process, "pending events process");
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  LOGA((void*)data, "App: received");
}
/*---------------------------------------------------------------------------*/
#define MAX_PRB 128
static struct ringbufindex log_ringbuf;
static uint32_t log_array[MAX_PRB];
static int log_dropped = 0;
void
app_probing_received(struct app_data *data)
{
  int log_index = ringbufindex_peek_put(&log_ringbuf);
  if(data && log_index != -1) {
    uint32_t *seqno = &log_array[log_index];
    struct app_data appdata;
    appdata_copy(&appdata, data);
    *seqno = appdata.seqno;
    ringbufindex_put(&log_ringbuf);
    process_poll(&pending_events_process);
    return;
  } else {
    log_dropped++;
    return;
  }
}
/*---------------------------------------------------------------------------*/
void
process_pending()
{
  static int last_log_dropped = 0;
  int16_t log_index;
  if(log_dropped != last_log_dropped) {
    LOG("App:! prb dropped %u\n", log_dropped);
    last_log_dropped = log_dropped;
  }
  while((log_index = ringbufindex_peek_get(&log_ringbuf)) != -1) {
    uint32_t *seqno = &log_array[log_index];
    struct app_data appdata;
    appdata.magic = UIP_HTONL(LOG_MAGIC);
    appdata.seqno = *seqno;
    appdata.src = UIP_HTONS(UIP_HTONL(*seqno) >> 16);
    appdata.dest = 0;
    appdata.hop = 0;
    LOGA((void*)&appdata, "App: received");
    /* Remove input from ringbuf */
    ringbufindex_get(&log_ringbuf);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(pending_events_process, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    process_pending();
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
app_send_broadcast()
{
  static unsigned int cnt;
  struct app_data data;
  uip_ipaddr_t dest_ipaddr;

  data.magic = UIP_HTONL(LOG_MAGIC);

  data.seqno = UIP_HTONL(((uint32_t)node_id << 16) + cnt);
  data.src = UIP_HTONS(node_id);
  data.dest = 0;
  data.hop = 0;

  LOGA(&data, "App: sending");

  uip_create_linklocal_allnodes_mcast(&dest_ipaddr);
  simple_udp_sendto(&broadcast_connection, &data, sizeof(data), &dest_ipaddr);

  cnt++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t global_ipaddr;

  PROCESS_BEGIN();

  node_id_restore();
#if WITH_TSCH
  if(node_id == COORDINATOR_ID) {
    tsch_is_coordinator = 1;
  }
#endif

  if(deployment_init(&global_ipaddr, NULL, -1)) {
    printf("App: %u starting\n", node_id);
  } else {
    printf("App: %u *not* starting\n", node_id);
    PROCESS_EXIT();
  }

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);
#if WITH_TSCH
#if WITH_OFFLINE_SCHEDULE_DEDICATED_PROBING
  /* 1: dedicated tx slot for each */
  struct tsch_slotframe *sf1 = tsch_schedule_add_slotframe(1, 103);
  tsch_schedule_add_link(sf1,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      (node_index*5)%103, 0);
  /* 2: wakeup at every slot */
  struct tsch_slotframe *sf2 = tsch_schedule_add_slotframe(2, 1);
  tsch_schedule_add_link(sf2,
        LINK_OPTION_RX,
        LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
        0, 0);
#elif WITH_OFFLINE_SCHEDULE_MINIMAL_PROBING
  /* 1: periodic shared EB timeslot */
    struct tsch_slotframe *sf1 = tsch_schedule_add_slotframe(1, 5);
    tsch_schedule_add_link(sf1,
        LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
        LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
        0, 0);
    /* 2: wakeup at every slot */
    struct tsch_slotframe *sf2 = tsch_schedule_add_slotframe(2, 1);
    tsch_schedule_add_link(sf2,
        LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
          LINK_TYPE_NORMAL, &tsch_broadcast_address,
          0, 0);
#else
#error Unsupported configuration
#endif
#endif

  ringbufindex_init(&log_ringbuf, MAX_PRB);
  process_start(&pending_events_process, NULL);
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    etimer_set(&send_timer, random_rand() % (SEND_INTERVAL));
    PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));

    app_send_broadcast();

    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
