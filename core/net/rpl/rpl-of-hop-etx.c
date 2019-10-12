/**
 * \addtogroup uip6
 * @{
 */
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
 *         A simple OF+metric that selects the shortest hop count to the root
 *         and excludes all links with an ETX above a threshold.
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

static void reset(rpl_dag_t *);
static void neighbor_link_callback(rpl_parent_t *, int, int);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_dag_t *best_dag(rpl_dag_t *, rpl_dag_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
static void update_metric_container(rpl_instance_t *);

rpl_of_t rpl_of_hop_etx = {
  reset,
  neighbor_link_callback,
  best_parent,
  best_dag,
  calculate_rank,
  update_metric_container,
  1
};

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Constants for the ETX moving average */
#define ETX_SCALE   100
#define ETX_ALPHA   90
/* Parent with less than ETX_EARLY_THRESHOLD Tx history use a more
 * aggressive alpha of ETX_EARLY_ALPHA */
#define ETX_EARLY_THRESHOLD   2
#define ETX_EARLY_ALPHA      70
/* Transmissions with more than ETX_BADLINK_THRESHOLD Tx count
 * use a more aggressive alpha of ETX_BADLINKY_ALPHA */
//#define ETX_BADLINK_THRESHOLD   3
//#define ETX_BADLINK_ALPHA      80
/* Non-acked transmissions translate to an ETX of NOACK_ETX_PENALTY */
#define NOACK_ETX_PENALTY     16

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST     100

#ifdef RPL_OF_HOP_ETX_CONF_THRESHOLD
#define RPL_OF_HOP_ETX_THRESHOLD RPL_OF_HOP_ETX_CONF_THRESHOLD
#else
#define RPL_OF_HOP_ETX_THRESHOLD (RPL_DAG_MC_ETX_DIVISOR / 2)
#endif

typedef uint16_t rpl_path_metric_t;

static void
reset(rpl_dag_t *sag)
{
}

static void
neighbor_link_callback(rpl_parent_t *p, int status, int numtx)
{
  uint16_t recorded_etx = p->link_metric;
  uint16_t packet_etx = numtx * RPL_DAG_MC_ETX_DIVISOR;
  uint16_t new_etx;

  /* Do not penalize the ETX when collisions or transmission errors occur. */
  if(status == MAC_TX_OK || status == MAC_TX_NOACK) {
    int etx_alpha = ETX_ALPHA;

    if(p->tx_count < ETX_EARLY_THRESHOLD) {
      etx_alpha = MIN(etx_alpha, ETX_EARLY_ALPHA);
    }

//    if(numtx > ETX_BADLINK_THRESHOLD) {
//      etx_alpha = MIN(etx_alpha, ETX_BADLINK_ALPHA);
//    }

    if(status == MAC_TX_NOACK) {
      packet_etx = NOACK_ETX_PENALTY * RPL_DAG_MC_ETX_DIVISOR;
    }

    new_etx = ((uint32_t)recorded_etx * etx_alpha +
               (uint32_t)packet_etx * (ETX_SCALE - etx_alpha)) / ETX_SCALE;

    PRINTF("RPL: ETX changed from %u to %u (packet ETX = %u)\n",
        (unsigned)(recorded_etx / RPL_DAG_MC_ETX_DIVISOR),
        (unsigned)(new_etx  / RPL_DAG_MC_ETX_DIVISOR),
        (unsigned)(packet_etx / RPL_DAG_MC_ETX_DIVISOR));
    p->link_metric = new_etx;
  }
}

static rpl_rank_t
calculate_rank(rpl_parent_t *p, rpl_rank_t base_rank)
{
  if(p == NULL) {
    return INFINITE_RANK;
  } else {
    return p->rank + RPL_MIN_HOPRANKINC;
  }
}

static rpl_dag_t *
best_dag(rpl_dag_t *d1, rpl_dag_t *d2)
{
  if(d1->grounded != d2->grounded) {
    return d1->grounded ? d1 : d2;
  }

  if(d1->preference != d2->preference) {
    return d1->preference > d2->preference ? d1 : d2;
  }

  return d1->rank < d2->rank ? d1 : d2;
}

rpl_parent_t *
rpl_of_hop_etx_select_parent()
{
  rpl_parent_t *p;
  rpl_parent_t *best_link_p;
  rpl_parent_t *best_p;

  /* First, look for the parent with the best link */
  p = (rpl_parent_t *)nbr_table_head(rpl_parents);

  if(p == NULL) {
    return NULL;
  }

  best_link_p = p;
  while(p != NULL) {
    if(p->rank == INFINITE_RANK) {
      /* ignore this neighbor */
    } else if(p->link_metric < best_link_p->link_metric) {
      best_link_p = p;
    }
    p = nbr_table_next(rpl_parents, p);
  }

  /* Second, look for the parent with the best rank among the
   * parents with a link better than best_link+LINK_METRIC_THRESHOLD */
  p = (rpl_parent_t *)nbr_table_head(rpl_parents);
  best_p = best_link_p;
  while(p != NULL) {
    if(p->rank == INFINITE_RANK) {
      /* ignore this neighbor */
    } else if(p->link_metric < best_link_p->link_metric+RPL_OF_HOP_ETX_THRESHOLD
        && p->rank < best_p->rank) {
      best_p = p;
    }
    p = nbr_table_next(rpl_parents, p);
  }

  printf("RPL: Best link %u %u, best p %u %u\n",
      best_link_p->link_metric, best_link_p->rank,
      best_p->link_metric, best_p->rank
  );

  return best_p;
}

static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  return NULL;
}

#if RPL_DAG_MC == RPL_DAG_MC_NONE
static void
update_metric_container(rpl_instance_t *instance)
{
  instance->mc.type = RPL_DAG_MC;
}
#else
static void
update_metric_container(rpl_instance_t *instance)
{
  rpl_path_metric_t path_metric;
  rpl_dag_t *dag;
#if RPL_DAG_MC == RPL_DAG_MC_ENERGY
  uint8_t type;
#endif

  instance->mc.type = RPL_DAG_MC;
  instance->mc.flags = RPL_DAG_MC_FLAG_P;
  instance->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
  instance->mc.prec = 0;

  dag = instance->current_dag;

  if (!dag->joined) {
    PRINTF("RPL: Cannot update the metric container when not joined\n");
    return;
  }

  if(dag->rank == ROOT_RANK(instance)) {
    path_metric = 0;
  } else {
    path_metric = calculate_path_metric(dag->preferred_parent);
  }

#if RPL_DAG_MC == RPL_DAG_MC_ETX
  instance->mc.length = sizeof(instance->mc.obj.etx);
  instance->mc.obj.etx = path_metric;

  PRINTF("RPL: My path ETX to the root is %u.%u\n",
	instance->mc.obj.etx / RPL_DAG_MC_ETX_DIVISOR,
	(instance->mc.obj.etx % RPL_DAG_MC_ETX_DIVISOR * 100) /
	 RPL_DAG_MC_ETX_DIVISOR);
#elif RPL_DAG_MC == RPL_DAG_MC_ENERGY
  instance->mc.length = sizeof(instance->mc.obj.energy);

  if(dag->rank == ROOT_RANK(instance)) {
    type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
  } else {
    type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
  }

  instance->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
  instance->mc.obj.energy.energy_est = path_metric;
#endif /* RPL_DAG_MC == RPL_DAG_MC_ETX */
}
#endif /* RPL_DAG_MC == RPL_DAG_MC_NONE */
