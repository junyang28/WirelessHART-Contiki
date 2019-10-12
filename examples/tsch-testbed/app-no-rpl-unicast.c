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
#include "net/mac/tsch/tsch-schedule.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/mac/tsch/tsch.h"
#include "net/ip/uip-debug.h"
#include "lib/random.h"
#include "deployment.h"
#include "simple-udp.h"
#include <stdio.h>
#include <string.h>

#define SEND_INTERVAL   (5 * CLOCK_SECOND)
#define UDP_PORT 1234

#define COORDINATOR_ID 2
#define DEST_ID 2
#define SRC_ID 3
#define WITH_PONG 1

static struct simple_udp_connection unicast_connection;
static uint16_t current_cnt = 0;
static uip_ipaddr_t llprefix;

/*---------------------------------------------------------------------------*/
PROCESS(unicast_sender_process, "No-RPL Unicast Application");
AUTOSTART_PROCESSES(&unicast_sender_process);
/*---------------------------------------------------------------------------*/
void app_send_to(uint16_t id, int ping, uint32_t seqno);
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *dataptr,
         uint16_t datalen)
{
  struct app_data data;
  appdata_copy((void *)&data, (void *)dataptr);
  if(data.ping) {
    LOGA((struct app_data *)dataptr, "App: received ping");
#if WITH_PONG
    app_send_to(data.src, 0, data.seqno | 0x8000l);
#endif
  } else {
    LOGA((struct app_data *)dataptr, "App: received pong");
  }
}
/*---------------------------------------------------------------------------*/
void
app_send_to(uint16_t id, int ping, uint32_t seqno)
{
  struct app_data data;
  uip_ipaddr_t dest_ipaddr;
  uip_lladdr_t dest_lladdr;

  data.magic = UIP_HTONL(LOG_MAGIC);
  data.seqno = seqno;
  data.src = node_id;
  data.dest = id;
  data.hop = 0;
  data.ping = ping;

  if(ping) {
    LOGA(&data, "App: sending ping");
  } else {
    LOGA(&data, "App: sending pong");
  }
  set_ipaddr_from_id(&dest_ipaddr, id);
  /* Convert global address into link-local */
  memcpy(&dest_ipaddr, &llprefix, 8);
  set_linkaddr_from_id((linkaddr_t *)&dest_lladdr, id);
  uip_ds6_nbr_add(&dest_ipaddr, &dest_lladdr, 1, ADDR_MANUAL);

  simple_udp_sendto(&unicast_connection, &data, sizeof(data), &dest_ipaddr);
}
/*---------------------------------------------------------------------------*/
/* DEBUG -- TESTING */
static void
test_memb(void)
{
  /* Struct definitions */
  /* 14B packed but 20 aligned */
  struct test_14b {
    uint16_t a[3]; //3*2
    uint16_t b; //2
    uint8_t c; //1
    uint32_t d; //4
    uint8_t e; //1
    linkaddr_t addr;
  };

  struct test_12b
  {
    uint16_t a; //2
    linkaddr_t addr;
    uint16_t b[3]; //3*2
    uint32_t c; //4
  };

  /* MEMB definitions and init */
#include "lib/memb.h"
#define SIZE (3)
  MEMB(test_14b_memb, struct test_14b, SIZE);
  MEMB(test_12b_memb, struct test_12b, SIZE);
  memb_init(&test_14b_memb);
  memb_init(&test_12b_memb);

  /* Alloc and free tests */
  struct test_14b *t14b[SIZE];
  struct test_12b *t12b[SIZE];
  int i;
  for(i = 0; i < SIZE; i++) {
    t14b[i] = memb_alloc(&test_14b_memb);
    printf("%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n",
        "&a16_arr: 3", &t14b[i]->a,
        "&a16_arr[0]", t14b[i]->a,
        "b16", &t14b[i]->b,
        "c8", &t14b[i]->c,
        "d32", &t14b[i]->d,
        "e8", &t14b[i]->e,
        "addr", &t14b[i]->addr
        );
  }
  for(i = 0; i < SIZE; i++) {
    memb_free(&test_14b_memb, t14b[i]);
  }

  for(i = 0; i < SIZE; i++) {
    t12b[i] = memb_alloc(&test_12b_memb);
    printf("%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n%s @ %p\n",
        "a16", &t12b[i]->a,
        "addr", &t12b[i]->addr,
        "&b16: 3", &t12b[i]->b,
        "&b16[0]", t12b[i]->b,
        "c32", &t12b[i]->c
        );
  }
  for(i = 0; i < SIZE; i++) {
    memb_free(&test_12b_memb, t12b[i]);
  }
}
static void
w_memcpy(uint8_t* dest, const uint8_t* src, const uint32_t n)
{
  uint32_t i;
  for(i = 0; i < n; i++) {
    dest[i] = src[i];
  }
}

static void
test_memcpy(void)
{
#include <string.h>
#define BLOCK_SIZE 128
  struct blocks32 {
    uint32_t aligned_block1[BLOCK_SIZE/4 + BLOCK_SIZE%4];
    uint32_t aligned_block2[BLOCK_SIZE/4 + BLOCK_SIZE%4];
    uint32_t aligned_block3[BLOCK_SIZE/4 + BLOCK_SIZE%4];
  };
  struct blocks32 block;
  uint8_t *p1, *p2, *p3, *src;
  int i, j;

  p1 = (uint8_t *)block.aligned_block1;
  p2 = (uint8_t *)block.aligned_block2;
  p3 = (uint8_t *)block.aligned_block3;

  memset((void *)p1, 0xaa, BLOCK_SIZE);
  memset((void *)p2, 0xbb, BLOCK_SIZE);
  memset((void *)p3, 0xcc, BLOCK_SIZE);

  for(i = 1; i < 10; i++) {
    src = p2 + i;
    printf("memcpy   i = 0x%x, src: 0x%p, dest: 0x%p\n", i, src, p1 + i);
    memcpy(p1 + i-1, src, i);
    for(j = 0; j < BLOCK_SIZE; j++) {
      printf("%x ", ((uint8_t *)block.aligned_block1)[j]);
    }
    printf("\n");
    printf("w_memcpy i = 0x%x, src: 0x%p, dest: 0x%p\n", i, src, p3 + i);
    w_memcpy(p3 + i-1, src, i);
    for(j = 0; j < BLOCK_SIZE; j++) {
      printf("%x ", ((uint8_t *)block.aligned_block3)[j]);
    }
    printf("\n");
    p1 += i;
    p2 += i;
    p3 += i;
  }
}
/* Profiling 64bit mod operation */
#include "net/mac/tsch/tsch-private.h"
uint16_t
test_mod(uint16_t a, uint8_t d)
{
  printf("Profiling 64bit mod operation\n");
  uint16_t c = 0;
  uint64_t b64 = 0xaaaaaaaaaaaaaaaaULL;
  uint32_t b32 = 0xbbbbbbbbUL;
  uint16_t b16 = 0xcccc;
  uint8_t b8 = 0xdd;
  rtimer_clock_t t0;
  t0 = RTIMER_NOW();
  c = (b64 + d) % a;
  t0 = RTIMER_NOW() - t0;
  printf("uint64_t mod uint64_t duration %u ticks = %u us c = %u\n", t0, (uint16_t)RTIMERTICKS_TO_US(t0), c);
  t0 = RTIMER_NOW();
  c = (b32 + d) % a;
  t0 = RTIMER_NOW() - t0;
  printf("uint64_t mod uint32_t duration %u ticks = %u us c = %u\n", t0, (uint16_t)RTIMERTICKS_TO_US(t0), c);
  t0 = RTIMER_NOW();
  c = (b16 + d) % a;
  t0 = RTIMER_NOW() - t0;
  printf("uint64_t mod uint16_t duration %u ticks = %u us c = %u\n", t0, (uint16_t)RTIMERTICKS_TO_US(t0), c);
  t0 = RTIMER_NOW();
  c = (b8 + d) % a;
  t0 = RTIMER_NOW() - t0;
  printf("uint64_t mod uint8_t duration %u ticks = %u us c = %u\n", t0, (uint16_t)RTIMERTICKS_TO_US(t0), c);
  return c;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_sender_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t global_ipaddr;

  PROCESS_BEGIN();

  node_id_restore();
  if(node_id == COORDINATOR_ID) {
    tsch_is_coordinator = 1;
  }

  if(deployment_init(&global_ipaddr, NULL, -1)) {
    printf("App: %u starting\n", node_id);
  } else {
    printf("App: %u *not* starting\n", node_id);
    PROCESS_EXIT();
  }

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  tsch_schedule_create_minimal();

  if(node_id != DEST_ID) {
    etimer_set(&periodic_timer, SEND_INTERVAL);
    while(1) {
      app_send_to(DEST_ID, 1, ((uint32_t)node_id << 16) + current_cnt);
      current_cnt++;

      PROCESS_WAIT_UNTIL(etimer_expired(&periodic_timer));
      etimer_reset(&periodic_timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
