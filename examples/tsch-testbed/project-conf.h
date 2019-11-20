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
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#include "deployment-def.h"

#define WITH_RPL 1
#define WITH_APP_PROBING (!WITH_RPL)
#define TSCH_CONF_EB_AUTOSELECT (!WITH_RPL)

/* #define WITH_OFFLINE_SCHEDULE_MINIMAL_PROBING 1 */
/* #define WITH_OFFLINE_SCHEDULE_DEDICATED_PROBING 1 */

#define WITH_ORCHESTRA 1
#define ORCHESTRA_MINIMAL_SCHEDULE 0
#define ORCHESTRA_RECEIVER_BASED   1
#define ORCHESTRA_SENDER_BASED     2
#define ORCHESTRA_CONFIG ORCHESTRA_SENDER_BASED
//#define ORCHESTRA_CONFIG ORCHESTRA_RECEIVER_BASED
//#define ORCHESTRA_CONFIG ORCHESTRA_MINIMAL_SCHEDULE

#if ORCHESTRA_CONFIG == ORCHESTRA_MINIMAL_SCHEDULE

#define TSCH_CONF_PACKET_DEST_ADDR_IN_ACK 1
#define TSCH_CONF_MIN_EB_PERIOD (4 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (16 * CLOCK_SECOND)
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

#elif ORCHESTRA_CONFIG == ORCHESTRA_RECEIVER_BASED

#define ORCHESTRA_UNICAST_PERIOD 7
#define TSCH_CONF_PACKET_DEST_ADDR_IN_ACK 1
#define TSCH_CALLBACK_NEW_TIME_SOURCE orchestra_callback_new_time_source
#define TSCH_CONF_MIN_EB_PERIOD (2 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (2 * CLOCK_SECOND)

#define NODE_INDEX_SUFFLE 1
#define NODE_INDEX_SUFFLE_MULTIPLICATOR 10
#define NODE_INDEX_SUFFLE_MODULUS MAX_NODES

#elif ORCHESTRA_CONFIG == ORCHESTRA_SENDER_BASED

#define ORCHESTRA_UNICAST_PERIOD 253
//#define ORCHESTRA_UNICAST_PERIOD2 0
#define TSCH_CONF_PACKET_DEST_ADDR_IN_ACK (ORCHESTRA_UNICAST_PERIOD < MAX_NODES)
#define TSCH_CALLBACK_NEW_TIME_SOURCE orchestra_callback_new_time_source
#define TSCH_CALLBACK_JOINING_NETWORK orchestra_callback_joining_network
//#define TSCH_CALLBACK_DO_NACK orchestra_callback_do_nack
#define TSCH_CONF_MIN_EB_PERIOD (2 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (2 * CLOCK_SECOND)

#define NODE_INDEX_SUFFLE 0
#define NODE_INDEX_SUFFLE_MULTIPLICATOR 1
#define NODE_INDEX_SUFFLE_MODULUS MAX_NODES

#endif

#if WITH_RPL && CONFIG == CONFIG_TSCH
#ifndef TSCH_CALLBACK_JOINING_NETWORK
#define TSCH_CALLBACK_JOINING_NETWORK tsch_rpl_callback_joining_network
#endif
#ifndef TSCH_CALLBACK_LEAVING_NETWORK
#define TSCH_CALLBACK_LEAVING_NETWORK tsch_rpl_callback_leaving_network
#endif
#define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
#define RPL_CALLBACK_NEW_DIO_INTERVAL tsch_rpl_callback_new_dio_interval
#endif

#define TSCH_CONF_GUARD_TIME 600

/* #define WITH_OF_HOP_ETX 1 */
/* #define WITH_OF_PDR 1 */
#define WITH_OF_ETX_EXP 1

#define TSCH_SCHEDULE_CONF_PRIORITIZE_TX 0
#define TSCH_CONF_USE_SFD_FOR_SYNC !IN_COOJA

#define TSCH_CONF_CHECK_TIME_AT_ASSOCIATION 20
#define RPL_CONF_PROBING 1
#define RPL_CONF_PROBING_TX_THRESHOLD 4 /* Stop probing after 4 tx to a neighbor */
#define RPL_CONF_PROBING_LOCK_ALL 1
#define RPL_CONF_RSSI_BASED_ETX 1

#define ANNOTATE_DEFAULT_ROUTE IN_COOJA

#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 16

#undef TSCH_CONF_QUEUE_NUM_PER_NEIGHBOR
#define TSCH_CONF_QUEUE_NUM_PER_NEIGHBOR 16

#undef TSCH_CONF_QUEUE_MAX_NEIGHBOR_QUEUES
#define TSCH_CONF_QUEUE_MAX_NEIGHBOR_QUEUES 6

#if !CONTIKI_TARGET_JN5168
#undef ENABLE_COOJA_DEBUG
#define ENABLE_COOJA_DEBUG 0
/* Contiki netstack: RADIO */
#undef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   cc2420_driver

/* Needed for TSCH */
#undef CC2420_CONF_SFD_TIMESTAMPS
#define CC2420_CONF_SFD_TIMESTAMPS 1

/* Needed for TSCH */
#undef DCOSYNCH_CONF_ENABLED
#define DCOSYNCH_CONF_ENABLED 0

/* The 802.15.4 PAN-ID */
#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID CMD_IEEE802154_PANID

/* The cc2420 transmission power (min:0, max: 31) */
#define RF_POWER                31

/* The cc2420 RSSI threshold (-32 is the reset value for -77 dBm) */
#define RSSI_THR        (-32 - 14)

#else

#undef ENABLE_COOJA_DEBUG
#define ENABLE_COOJA_DEBUG 0

/* max 3, min 0 */
#undef RF_POWER
#define RF_POWER               3

#undef USE_SLIP_UART1
#define USE_SLIP_UART1 0

#undef UART_HW_FLOW_CTRL
#define UART_HW_FLOW_CTRL 0

#endif /* !CONTIKI_TARGET_JN5168 */

#include "cooja-debug.h"

/* No Downwards routes */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NO_DOWNWARD_ROUTES
//#define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST

#undef UIP_CONF_IP_FORWARD
#define UIP_CONF_IP_FORWARD 0

/* RPL Trickle timer tuning */
#undef RPL_CONF_DIO_INTERVAL_MIN
#define RPL_CONF_DIO_INTERVAL_MIN 12 /* 4.096 s */

#undef RPL_CONF_DIO_INTERVAL_DOUBLINGS
//#define RPL_CONF_DIO_INTERVAL_DOUBLINGS 4 /* Max factor: x16. 4.096 s * 16 = 65.536 s */
#define RPL_CONF_DIO_INTERVAL_DOUBLINGS 6 /* Max factor: x64. 4.096 s * 64 = 262.144 s */

/* No channel hopping for the moment */
#undef TSCH_CONF_N_CHANNELS
#define TSCH_CONF_N_CHANNELS 5

#if WITH_OF_PDR

#undef RPL_CONF_OF
#define RPL_CONF_OF rpl_of_pdr
#undef RPL_CONF_INIT_LINK_METRIC
#define RPL_CONF_INIT_LINK_METRIC (64256/RPL_DAG_MC_ETX_DIVISOR) /* 98% PRR */
#undef RPL_CONF_MIN_HOPRANKINC
#define RPL_CONF_MIN_HOPRANKINC 1
#undef RPL_CONF_MAX_HOPRANKINC
#define RPL_CONF_MAX_HOPRANKINC 0
//#define RPL_CONF_MAX_NBRHOPINC

#elif WITH_OF_HOP_ETX

#undef RPL_CONF_OF
#define RPL_CONF_OF rpl_of_hop_etx
#undef RPL_CONF_INIT_LINK_METRIC
#define RPL_CONF_INIT_LINK_METRIC 1
#undef RPL_CONF_MIN_HOPRANKINC
#define RPL_CONF_MIN_HOPRANKINC 1
#undef RPL_CONF_MAX_HOPRANKINC
#define RPL_CONF_MAX_HOPRANKINC 0
#define RPL_CONF_MAX_NBRHOPINC 4
#define RPL_OF_HOP_ETX_CONF_THRESHOLD (RPL_DAG_MC_ETX_DIVISOR / 2)

#elif WITH_OF_ETX_EXP

#undef RPL_CONF_OF
#define RPL_CONF_OF rpl_of_etx_exp
#undef RPL_CONF_INIT_LINK_METRIC
#define RPL_CONF_INIT_LINK_METRIC 2 /* default 5 */
#undef RPL_CONF_MIN_HOPRANKINC
#define RPL_CONF_MIN_HOPRANKINC 256
#undef RPL_CONF_MAX_HOPRANKINC
#define RPL_CONF_MAX_HOPRANKINC 0 /* default (7 * RPL_MIN_HOPRANKINC) */
/* Do not accept RPL neighbors with rank greater than ours + 1.5 */
#define RPL_CONF_MAX_NBRHOPINC (RPL_MIN_HOPRANKINC + RPL_MIN_HOPRANKINC/2)
/* Exponent applied to the link etx */
#define RPL_OF_ETX_EXP_CONF_N 2

#else /* WITH_OF_HOP_ETX */

/* Default link metric */
#undef RPL_CONF_INIT_LINK_METRIC
#define RPL_CONF_INIT_LINK_METRIC 2 /* default 5 */
#undef RPL_CONF_MIN_HOPRANKINC
#define RPL_CONF_MIN_HOPRANKINC 256
#undef RPL_CONF_MAX_HOPRANKINC
#define RPL_CONF_MAX_HOPRANKINC 0 /* default (7 * RPL_MIN_HOPRANKINC) */
/* Do not accept RPL neighbors with rank greater than ours + 1.5 */
#define RPL_CONF_MAX_NBRHOPINC (RPL_MIN_HOPRANKINC + RPL_MIN_HOPRANKINC/2)

#endif /* WITH_OF_HOP_ETX */

/* The neighbor table size */
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 22

/* The routing table size */
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES  0

/* No RPL DIS */
#undef RPL_CONF_DIS_SEND
#define RPL_CONF_DIS_SEND 0

/* Space saving */
#undef UIP_CONF_TCP
#define UIP_CONF_TCP             0
#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG     0
#undef UIP_CONF_DS6_ADDR_NBU
#define UIP_CONF_DS6_ADDR_NBU    1
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE   160
#undef UIP_CONF_UDP_CONNS
#define UIP_CONF_UDP_CONNS       2
#undef UIP_CONF_FWCACHE_SIZE
#define UIP_CONF_FWCACHE_SIZE    4

#undef PROCESS_CONF_NO_PROCESS_NAMES
#define PROCESS_CONF_NO_PROCESS_NAMES 1

/* Disable UDP checksum, needed as we have mutable fields (hop count) in the data packet */
#undef UIP_CONF_UDP_CHECKSUMS
#define UIP_CONF_UDP_CHECKSUMS   0

#define RPL_CONF_MAX_INSTANCES    1 /* default 1 */
#define RPL_CONF_MAX_DAG_PER_INSTANCE 1 /* default 2 */

#undef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 0

/* Contiki netstack: MAC */
#undef NETSTACK_CONF_MAC

/* Contiki netstack: RDC */
#undef NETSTACK_CONF_RDC

#if CONFIG == CONFIG_NULLRDC
/* set channel */
#undef RF_CHANNEL
#define RF_CHANNEL 26

/* radio conf: enable autoACK */
#undef MICROMAC_RADIO_CONF_AUTOACK
#define MICROMAC_RADIO_CONF_AUTOACK 1
#undef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK 1

/* increase internal radio buffering */
#undef RADIO_BUF_CONF_NUM
#define RADIO_BUF_CONF_NUM 4

#define NETSTACK_CONF_MAC     csma_driver
#define NETSTACK_CONF_RDC     nullrdc_driver
#define NULLRDC_CONF_802154_AUTOACK 1
#undef CSMA_CONF_MAX_MAC_TRANSMISSIONS
#define CSMA_CONF_MAX_MAC_TRANSMISSIONS 9

#elif CONFIG == CONFIG_CONTIKIMAC

#undef RF_CHANNEL
#define RF_CHANNEL 26
#undef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK 1
#define NETSTACK_CONF_MAC     csma_driver
#define NETSTACK_CONF_RDC     contikimac_driver
#undef CONTIKIMAC_CONF_COMPOWER
#define CONTIKIMAC_CONF_COMPOWER 0
//#define CONTIKIMAC_CONF_CYCLE_TIME (RTIMER_ARCH_SECOND / 22)
#undef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#undef CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION
#define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 1
#undef CSMA_CONF_MAX_MAC_TRANSMISSIONS
#define CSMA_CONF_MAX_MAC_TRANSMISSIONS 9
#undef CSMA_CONF_MAX_NEIGHBOR_QUEUES
#define CSMA_CONF_MAX_NEIGHBOR_QUEUES 6

#elif CONFIG == CONFIG_TSCH

#undef CC2420_CONF_SEND_CCA
#define CC2420_CONF_SEND_CCA 0
#undef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK 0
#define NETSTACK_CONF_MAC     tschmac_driver
#define NETSTACK_CONF_RDC     nordc_driver
#undef WITH_TSCH
#define WITH_TSCH 1

#else

#error Unsupported config

#endif

/* Contiki netstack: FRAMER */
#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154

#define WITH_DEPLOYMENT 1
#define WITH_TSCH_LOG 0
#define WITH_LOG 1
#define WITH_LOG_HOP_COUNT 1
#if WITH_LOG
#include "deployment-log.h"
#endif

#endif /* __PROJECT_CONF_H__ */
