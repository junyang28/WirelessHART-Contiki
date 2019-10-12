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
#include <stdio.h>

#define SEND_INTERVAL   (60 * CLOCK_SECOND)
#define UDP_PORT 1234

#define COORDINATOR_ID 2
#define DEST_ID 2
#define SRC_ID 3

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_sender_process, "No-RPL Broadcast Application");
AUTOSTART_PROCESSES(&broadcast_sender_process);
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
void
app_send_broadcast()
{

  static unsigned int cnt;
  struct app_data data;
  uip_ipaddr_t dest_ipaddr;

  data.magic = UIP_HTONL(LOG_MAGIC);
  data.seqno = ((uint32_t)node_id << 16) + cnt;
  data.src = node_id;
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
  tsch_schedule_create_minimal();
#endif

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    etimer_set(&send_timer, random_rand() % (SEND_INTERVAL));
    PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));

    if(node_id == SRC_ID) {
      app_send_broadcast();
    }
    PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
