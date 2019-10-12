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
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "net/rpl/rpl-private.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

static void reset(rpl_dag_t *);
static void neighbor_link_callback(rpl_parent_t *, int, int);
static rpl_parent_t *best_parent(rpl_parent_t *, rpl_parent_t *);
static rpl_dag_t *best_dag(rpl_dag_t *, rpl_dag_t *);
static rpl_rank_t calculate_rank(rpl_parent_t *, rpl_rank_t);
static void update_metric_container(rpl_instance_t *);

rpl_of_t rpl_of_pdr = {
  reset,
  neighbor_link_callback,
  best_parent,
  best_dag,
  calculate_rank,
  update_metric_container,
  1
};

/* Constants for the PRR moving average */
#define PRR_SCALE   100
#define PRR_ALPHA   98
#define PRR_EXPONENT 4

#define PDR_BASE 65536ul
#define PDR_ONE (PDR_BASE-1)
#define PDR_ZERO 0

#define PDR_TO_ETX(prr) (((prr) != 0) ? (RPL_DAG_MC_ETX_DIVISOR*PDR_ONE)/(prr) : 0)
#define ETX_TO_PDR(etx) (((etx) != 0) ? (RPL_DAG_MC_ETX_DIVISOR*PDR_ONE)/(etx) : 0)

/* Transmission failures count as this many transmissions attempts */
#define NOACK_TX_COUNT      10

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST			100

typedef uint16_t rpl_path_metric_t;

static void
reset(rpl_dag_t *sag)
{
}

static void
neighbor_link_callback(rpl_parent_t *p, int status, int numtx)
{
  /* Do not penalize the ETX when collisions or transmission errors occur. */
  if(status == MAC_TX_OK || status == MAC_TX_NOACK) {
    uint16_t recorded_prr = p->link_metric;
    uint16_t new_prr = recorded_prr;
    int i;

    /* Incorporate with numtx-1 failures */
    for(i=0; i<numtx-1; i++) {
      new_prr = ((uint32_t)new_prr * PRR_ALPHA) / PRR_SCALE;
    }
    /* Incorporate last tx */
    if(status == MAC_TX_NOACK) {
      /* One more failure */
      new_prr = ((uint32_t)new_prr * PRR_ALPHA) / PRR_SCALE;
    } else {
      /* One success */
      new_prr = ((uint32_t)new_prr * PRR_ALPHA +
                     (uint32_t)PDR_ONE * (PRR_SCALE - PRR_ALPHA)) / PRR_SCALE;
    }

    LOG("RPL: PRR of %u changed from %u to %u (st %u %u)\n",
        LOG_NODEID_FROM_IPADDR(rpl_get_parent_ipaddr(p)),
        recorded_prr, new_prr,
        status, numtx);

    p->link_metric = new_prr;
  }
}

static rpl_rank_t
calculate_rank(rpl_parent_t *p, rpl_rank_t base_rank)
{
  if(p == NULL) {
    return INFINITE_RANK;
  } else {
    uint32_t link_loss_rate = PDR_BASE - p->link_metric;
    uint32_t parent_pdr = (uint32_t)PDR_BASE-p->rank;
    int i;

    link_loss_rate = PDR_BASE;
    for(i=0; i<PRR_EXPONENT; i++) {
      link_loss_rate *= PDR_BASE - p->link_metric;
      link_loss_rate /= PDR_BASE;
    }

    uint32_t link_prr = PDR_ONE - link_loss_rate;
    uint32_t path_pdr = ((PDR_BASE/2)+parent_pdr*link_prr) / PDR_BASE;
    uint32_t path_loss_rate = PDR_ONE - path_pdr;

    if(path_loss_rate > p->rank + RPL_MIN_HOPRANKINC) {
      return path_loss_rate;
    } else {
      return p->rank + RPL_MIN_HOPRANKINC;
    }
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

static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  rpl_dag_t *dag;
  rpl_path_metric_t min_diff;
  rpl_path_metric_t p1_metric;
  rpl_path_metric_t p2_metric;

  dag = p1->dag; /* Both parents are in the same DAG. */

  /* No hysteresis for now */
  min_diff = 0;

  p1_metric = calculate_rank(p1,0);
  p2_metric = calculate_rank(p2,0);

  /* Maintain stability of the preferred parent in case of similar ranks. */
  if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
    if(p1_metric < p2_metric + min_diff &&
       p1_metric > p2_metric - min_diff) {
      PRINTF("RPL: hysteresis: %u <= %u <= %u\n",
             p2_metric - min_diff,
             p1_metric,
             p2_metric + min_diff);
      return dag->preferred_parent;
    }
  }

  return p1_metric < p2_metric ? p1 : p2;
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
