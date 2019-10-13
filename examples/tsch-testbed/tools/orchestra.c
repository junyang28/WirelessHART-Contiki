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
 *         Orchestra
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"

#include "lib/memb.h"
#include "net/packetbuf.h"
#include "net/rpl/rpl.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-private.h"
#include "net/mac/tsch/tsch-schedule.h"
#include "deployment.h"
#include "net/rime/rime.h"
#include "tools/orchestra.h"
#include <stdio.h>

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#if ORCHESTRA_WITH_EBSF
static struct tsch_slotframe *sf_eb;
#endif
#if ORCHESTRA_WITH_COMMON_SHARED
static struct tsch_slotframe *sf_common;
#endif
#if ORCHESTRA_WITH_RBUNICAST
static struct tsch_slotframe *sf_rb;
#endif
#if ORCHESTRA_WITH_SBUNICAST
static struct tsch_slotframe *sf_sb;
static struct tsch_slotframe *sf_sb2;
/* Delete dedicated slots after 2 minutes */
#define DEDICATED_SLOT_LIFETIME TSCH_CLOCK_TO_SLOTS(2 * 60 * CLOCK_SECOND)
/* A net-layer sniffer for packets sent and received */
static void orchestra_packet_received(void);
static void orchestra_packet_sent(int mac_status);
RIME_SNIFFER(orhcestra_sniffer, orchestra_packet_received, orchestra_packet_sent);
#endif

#if ORCHESTRA_WITH_SBUNICAST
struct link_timestamps {
  uint32_t last_tx;
  uint32_t last_rx;
};
MEMB(nbr_timestamps, struct link_timestamps, TSCH_MAX_LINKS);
/*---------------------------------------------------------------------------*/
int
orchestra_callback_do_nack(struct tsch_link *link, linkaddr_t *src, linkaddr_t *dst)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
orchestra_callback_joining_network_sf(struct tsch_slotframe *sf)
{
  /* Cleanup sf: reset timestamps. Old links remain active for DEDICATED_SLOT_LIFETIME */
  struct tsch_link *l = list_head(sf->links_list);
  while(l != NULL) {
    struct link_timestamps *ts = (struct link_timestamps *)l->data;
    if(ts != NULL) {
      ts->last_tx = ts->last_rx = current_asn.ls4b;
    }
    l = list_item_next(l);
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_joining_network(void)
{
  orchestra_callback_joining_network_sf(sf_sb);
#ifdef ORCHESTRA_SBUNICAST_PERIOD2
  orchestra_callback_joining_network_sf(sf_sb2);
#endif
  tsch_rpl_callback_joining_network();
}
/*---------------------------------------------------------------------------*/
static void
orchestra_delete_old_links_sf(struct tsch_slotframe *sf)
{
  struct tsch_link *l = list_head(sf->links_list);
  struct tsch_link *prev = NULL;
  /* Loop over all links and remove old ones. */
  while(l != NULL) {
    struct link_timestamps *ts = (struct link_timestamps *)l->data;
    if(ts != NULL) {
      int tx_outdated = current_asn.ls4b - ts->last_tx > DEDICATED_SLOT_LIFETIME;
      int rx_outdated = current_asn.ls4b - ts->last_rx > DEDICATED_SLOT_LIFETIME;
      if(tx_outdated && rx_outdated) {
        /* Link outdated both for tx and rx, delete */
        PRINTF("Orchestra: removing link at %u\n", l->timeslot);
        tsch_schedule_remove_link(sf, l);
        memb_free(&nbr_timestamps, ts);
        l = prev;
      } else if(!rx_outdated && tx_outdated && (l->link_options & LINK_OPTION_TX)) {
        PRINTF("Orchestra: removing tx flag at %u\n", l->timeslot);
        /* Link outdated for tx, update */
        tsch_schedule_add_link(sf,
            l->link_options & ~(LINK_OPTION_TX | LINK_OPTION_SHARED),
            LINK_TYPE_NORMAL, &linkaddr_null,
            l->timeslot, l->channel_offset);
      } else if(!tx_outdated && rx_outdated && (l->link_options & LINK_OPTION_RX)) {
        PRINTF("Orchestra: removing rx flag at %u\n", l->timeslot);
        /* Link outdated for rx, update */
        linkaddr_t link_addr;
        linkaddr_copy(&link_addr, &l->addr);
        tsch_schedule_add_link(sf,
            l->link_options & ~LINK_OPTION_RX,
            LINK_TYPE_NORMAL, &link_addr,
            l->timeslot, l->channel_offset);
      }
    }
    prev = l;
    l = list_item_next(l);
  }
}
/*---------------------------------------------------------------------------*/
static void
orchestra_delete_old_links()
{
  //orchestra_delete_old_links_sf(sf_sb);
#ifdef ORCHESTRA_SBUNICAST_PERIOD2
  orchestra_delete_old_links_sf(sf_sb2);
#endif
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_received(void)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }

  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  if(dest_id != 0) { /* Not a broadcast */
    uint16_t src_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_SENDER));
    uint16_t src_index = get_node_index_from_id(src_id);
    /* Successful unicast Rx
     * We schedule a Rx link to listen to the source's dedicated slot,
     * in all unicast SFs */
    linkaddr_t link_addr;
    uint8_t timeslot;
    uint8_t choffset;
    static struct tsch_slotframe *sf;
#ifdef ORCHESTRA_SBUNICAST_PERIOD2
    if(src_index < ORCHESTRA_SBUNICAST_PERIOD) {
      /* Use the first slotframe */
      timeslot = src_index;
      sf = sf_sb;
      choffset = 2;
    } else {
      timeslot = src_index - ORCHESTRA_SBUNICAST_PERIOD;
      sf = sf_sb2;
      choffset = 3;
    }
#else
    sf = sf_sb;
    timeslot = src_index % ORCHESTRA_SBUNICAST_PERIOD;
    choffset = 2;
#endif
    struct link_timestamps *ts;
    uint8_t link_options = LINK_OPTION_RX;
    struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf, timeslot);
    if(l == NULL) {
      linkaddr_copy(&link_addr, &linkaddr_null);
      ts = memb_alloc(&nbr_timestamps);
    } else {
      link_options |= l->link_options;
      linkaddr_copy(&link_addr, &l->addr);
      ts = l->data;
      if(link_options != l->link_options) {
        /* Link options have changed, update the link */
        l = NULL;
      }
    }
    /* Now add/update the link */
    if(l == NULL) {
      PRINTF("Orchestra: adding rx link at %u\n", timeslot);
      l = tsch_schedule_add_link(sf,
          link_options,
          LINK_TYPE_NORMAL, &link_addr,
          timeslot, choffset);
    } else {
      PRINTF("Orchestra: updating rx link at %u\n", timeslot);
    }
    /* Update Rx timestamp */
    if(l != NULL && ts != NULL) {
      ts->last_rx = current_asn.ls4b;
      l->data = ts;
    }
  }
  orchestra_delete_old_links();
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_sent(int mac_status)
{
  if(packetbuf_attr(PACKETBUF_ATTR_PROTO) == UIP_PROTO_ICMP6) {
    /* Filter out ICMP */
    return;
  }
  uint16_t dest_id = node_id_from_linkaddr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  uint16_t dest_index = get_node_index_from_id(dest_id);
  if(dest_index != 0xffff && mac_status == MAC_TX_OK) {
    /* Successful unicast Tx
     * We schedule a Tx link to this neighbor, in all unicast SFs */
    uint8_t timeslot;
    uint8_t choffset;
    static struct tsch_slotframe *sf;
#ifdef ORCHESTRA_SBUNICAST_PERIOD2
    if(node_index < ORCHESTRA_SBUNICAST_PERIOD) {
      /* Use the first slotframe */
      timeslot = node_index;
      sf = sf_sb;
      choffset = 2;
    } else {
      timeslot = node_index - ORCHESTRA_SBUNICAST_PERIOD;
      sf = sf_sb2;
      choffset = 3;
    }
#else
    //LOG("test\n");
    sf = sf_sb;
    choffset = 2;
    timeslot = node_index % ORCHESTRA_SBUNICAST_PERIOD;
#endif
    struct link_timestamps *ts;
    uint8_t link_options = LINK_OPTION_TX | (ORCHESTRA_SBUNICAST_SHARED ? LINK_OPTION_SHARED : 0);
    struct tsch_link *l = tsch_schedule_get_link_from_timeslot(sf, timeslot);
    if(l == NULL) {
      ts = memb_alloc(&nbr_timestamps);
    } else {
      link_options |= l->link_options;
      ts = l->data;
      if(link_options != l->link_options
          || !linkaddr_cmp(&l->addr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))) {
        /* Link options or address have changed, update the link */
        l = NULL;
      }
    }
    /* Now add/update the link */
    if(l == NULL) {
      PRINTF("Orchestra: adding tx link at %u\n", timeslot);
      l = tsch_schedule_add_link(sf,
          link_options,
          LINK_TYPE_NORMAL, packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
          timeslot, choffset);
    } else {
      PRINTF("Orchestra: update tx link at %u\n", timeslot);
    }
    /* Update Tx timestamp */
    if(l != NULL && ts != NULL) {
      ts->last_tx = current_asn.ls4b;
      l->data = ts;
    }
  }
  orchestra_delete_old_links();
}
#endif /* ORCHESTRA_WITH_SBUNICAST */

void
orchestra_callback_new_time_source(struct tsch_neighbor *old, struct tsch_neighbor *new)
{
  uint16_t old_id = node_id_from_linkaddr(&old->addr);
  uint16_t old_index = get_node_index_from_id(old_id);
  uint16_t new_id = node_id_from_linkaddr(&new->addr);
  uint16_t new_index = get_node_index_from_id(new_id);

  if(new_index == old_index) {
    return;
  }

#if ORCHESTRA_WITH_EBSF
  if(old_index != 0xffff) {
    PRINTF("Orchestra: removing rx link for %u (%u) EB\n", old_id, old_index);
    /* Stop listening to the old time source's EBs */
    tsch_schedule_remove_link_from_timeslot(sf_eb, old_index);
  }
  if(new_index != 0xffff) {
    /* Listen to the time source's EBs */
    PRINTF("Orchestra: adding rx link for %u (%u) EB\n", new_id, new_index);
    tsch_schedule_add_link(sf_eb,
        LINK_OPTION_RX,
        LINK_TYPE_ADVERTISING_ONLY, NULL,
        new_index, 0);
  }
#endif /* ORCHESTRA_WITH_EBSF */

}


int schedule[] = {
	3, 2, 1,
	2, 1, 3
};


void
orchestra_init()
{
#if ORCHESTRA_WITH_EBSF
  sf_eb = tsch_schedule_add_slotframe(0, ORCHESTRA_EBSF_PERIOD);
  /* EB link: every neighbor uses its own to avoid contention */
  tsch_schedule_add_link(sf_eb,
      LINK_OPTION_TX,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      node_index, 0);
#endif

//application packets
#if ORCHESTRA_WITH_SBUNICAST
  memb_init(&nbr_timestamps);
  /* Sender-based slotframe for unicast */
  sf_sb = tsch_schedule_add_slotframe(2, ORCHESTRA_SBUNICAST_PERIOD);

  /* Rx links (with lease time) will be added upon receiving unicast */
  /* Tx links (with lease time) will be added upon transmitting unicast (if ack received) */
  int i = 0;
  //LOG("orchestra: %d\n", sizeof(schedule)/sizeof(schedule[0]));
  for(i = 0; i < sizeof(schedule)/sizeof(schedule[0]); i = i + 3){
  	//LOG("node %d %d %d\n", schedule[i], schedule[i+1], schedule[i+2]);
  	if(schedule[i] == node_index){
  		//LOG("1    node %d timeslot %d\n", node_index, schedule[i+2]);
  		/*
  		tsch_schedule_add_link(sf_sb,
      		LINK_OPTION_TX ,
      		ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
      		schedule[i+2], 2);
		*/
  		tsch_schedule_add_link(sf_sb,
      		LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      		ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
      		schedule[i+2], 2);
  	}
  	else if(schedule[i+1] == node_index){
  		//LOG("2   node %d timeslot %d\n", node_index, schedule[i+2]);
  		tsch_schedule_add_link(sf_sb,
      		LINK_OPTION_RX ,
      		ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
      		schedule[i+2], 2);  	
  	}
  }


  //rime_sniffer_add(&orhcestra_sniffer);
#endif

#if ORCHESTRA_WITH_COMMON_SHARED
  /* Default slotframe: for broadcast or unicast to neighbors we
   * do not have a link to */
  /*
  sf_common = tsch_schedule_add_slotframe(1, ORCHESTRA_COMMON_SHARED_PERIOD);
  tsch_schedule_add_link(sf_common,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      ORCHESTRA_COMMON_SHARED_TYPE, &tsch_broadcast_address,
      0, 1);
   */
#endif
}
