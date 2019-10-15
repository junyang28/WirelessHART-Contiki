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
 *         Code managing id<->mac address<->IPv6 address mapping, and doing this
 *         for different deployment scenarios: Cooja, Nodes, Indriya or Twist testbeds
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki-conf.h"
#include "deployment.h"
#include "sys/node-id.h"
#include "net/rpl/rpl.h"
#include "net/mac/tsch/tsch.h"
#include "net/ip/uip-debug.h"
#include "random.h"
#include <string.h>
#include <stdio.h>
#if CONTIKI_TARGET_SKY || CONTIKI_TARGET_Z1
#include "cc2420.h"
#endif

#if WITH_DEPLOYMENT

#if IN_COOJA
#pragma message "deployment.c: compiling with flag IN_COOJA"
#endif
#if IN_MOTES
#pragma message "deployment.c: compiling with flag IN_MOTES"
#endif
#if IN_TWIST
#pragma message "deployment.c: compiling with flag IN_TWIST"
#endif
#if IN_INDRIYA
#pragma message "deployment.c: compiling with flag IN_INDRIYA"
#endif
#if IN_NESTESTBED
#pragma message "deployment.c: compiling with flag IN_NESTESTBED"
#endif

#ifndef WITH_TSCH
#define WITH_TSCH 1
#endif

/* Our absolute index in the id_mac table */
uint16_t node_index = 0xffff;

/* Our global IPv6 prefix */
static uip_ipaddr_t prefix;

/* ID<->MAC address mapping */
struct id_mac {
  uint16_t id;
  linkaddr_t mac;
};

/* List of ID<->MAC mapping used for different deployments */
static const struct id_mac id_mac_list[] = {
#if IN_INDRIYA
    {  1, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xb3,0xae}}},
    {  2, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x40,0x7e}}},
    {  3, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xed,0x11}}},
    {  4, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xdb,0xf3}}},
    {  5, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x72,0x34}}},
    {  6, {{0x00,0x12,0x74,0x00,0x14,0x65,0xb9,0x16}}},
    {  7, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x87,0x98}}},
    {  8, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x58,0x65}}},
    {  9, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5f,0x65}}},
    { 10, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x56,0xf7}}},
    { 11, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x77,0x76}}},
    { 12, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x99,0xa6}}},
    { 13, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x99,0x1b}}},
    { 14, {{0x00,0x12,0x74,0x00,0x13,0xea,0x2b,0xb1}}},
    { 15, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x88,0x91}}},
    { 16, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x11,0xe6}}},
    { 17, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x60,0x11}}},
    { 18, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x90,0x21}}},
    { 20, {{0x00,0x12,0x74,0x00,0x14,0x65,0xcc,0xb6}}},
    { 21, {{0x00,0x12,0x74,0x00,0x14,0x65,0xc5,0x10}}},
    { 22, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xa8,0x17}}},
    { 24, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xcc,0x14}}},
    { 25, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x3f,0x4a}}},
    { 26, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xac,0x3f}}},
    { 27, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x9d,0xf4}}},
    { 28, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xd8,0xb2}}},
    { 30, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x7d,0xc0}}},
    { 31, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5f,0x0d}}},
    { 32, {{0x00,0x12,0x74,0x00,0x12,0xe6,0xa3,0xb0}}},
    { 33, {{0x00,0x12,0x74,0x00,0x14,0x65,0xd8,0xb5}}},
    { 34, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x56,0x51}}},
    { 35, {{0x00,0x12,0x74,0x00,0x12,0xe6,0xb0,0x63}}},
    { 36, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x0c,0x26}}},
    { 37, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x86,0x95}}},
    { 38, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x21,0x1b}}},
    { 39, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x48,0x7e}}},
    { 40, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xf3,0x2a}}},
    { 41, {{0x00,0x12,0x74,0x00,0x0e,0xd7,0xe2,0x98}}},
    { 42, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xee,0x0e}}},
    { 43, {{0x00,0x12,0x74,0x00,0x14,0x6f,0x0f,0x75}}},
    { 44, {{0x00,0x12,0x74,0x00,0x12,0xe6,0xa1,0x5d}}},
    { 45, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x56,0x08}}},
    { 46, {{0x00,0x12,0x74,0x00,0x13,0xea,0x4c,0x4e}}},
    { 47, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x78,0x8f}}},
    { 48, {{0x00,0x12,0x74,0x00,0x14,0x6f,0x0b,0x2f}}},
    { 50, {{0x00,0x12,0x74,0x00,0x14,0x65,0xc4,0xa9}}},
    { 51, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5b,0xfa}}},
    { 52, {{0x00,0x12,0x74,0x00,0x14,0x65,0xc2,0x65}}},
    { 53, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xcd,0x83}}},
    { 54, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x34,0xd6}}},
    { 55, {{0x00,0x12,0x74,0x00,0x13,0xea,0x21,0x4d}}},
    { 56, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xb4,0x61}}},
    { 57, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x77,0xdc}}},
    { 58, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x93,0xd3}}},
    { 59, {{0x00,0x12,0x74,0x00,0x13,0xea,0x29,0xd4}}},
    { 60, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5d,0xcd}}},
    { 63, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x2a,0x36}}},
    { 64, {{0x00,0x12,0x74,0x00,0x13,0xe9,0x9a,0x84}}},
    { 65, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x83,0x00}}},
    { 66, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x87,0x8b}}},
    { 67, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xd9,0x3e}}},
    { 68, {{0x00,0x12,0x74,0x00,0x13,0xea,0x46,0xf0}}},
    { 69, {{0x00,0x12,0x74,0x00,0x13,0xea,0x1c,0x26}}},
    { 70, {{0x00,0x12,0x74,0x00,0x13,0xea,0x45,0xc9}}},
    { 71, {{0x00,0x12,0x74,0x00,0x13,0xea,0x45,0xb2}}},
    { 73, {{0x00,0x12,0x74,0x00,0x13,0xea,0x5f,0xb2}}},
    { 74, {{0x00,0x12,0x74,0x00,0x0e,0xd8,0x09,0xac}}},
    { 75, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x78,0x6d}}},
    { 76, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5c,0xfa}}},
    { 77, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xc3,0xb8}}},
    { 78, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x8a,0xf5}}},
    { 79, {{0x00,0x12,0x74,0x00,0x14,0x6f,0x04,0xe8}}},
    { 80, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x7d,0xc8}}},
    { 81, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x75,0x8c}}},
    { 82, {{0x00,0x12,0x74,0x00,0x13,0xea,0x4e,0xa2}}},
    { 84, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x5d,0xc7}}},
    { 85, {{0x00,0x12,0x74,0x00,0x0e,0xd5,0x5c,0xa9}}},
    {115, {{0x00,0x12,0x74,0x00,0x12,0xe6,0xb0,0x9b}}},
    {116, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xf2,0x56}}},
    {117, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xd1,0x40}}},
    {118, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xe5,0xbd}}},
    {119, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x3b,0xb1}}},
    {120, {{0x00,0x12,0x74,0x00,0x12,0xe5,0xd3,0xc5}}},
    {121, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x4e,0xb5}}},
    {122, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xed,0x7a}}},
    {123, {{0x00,0x12,0x74,0x00,0x14,0x65,0xcd,0x82}}},
    {124, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xeb,0xb8}}},
    {126, {{0x00,0x12,0x74,0x00,0x12,0xe6,0xab,0x4e}}},
    {127, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x44,0xdc}}},
    {128, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xef,0x2d}}},
    {129, {{0x00,0x12,0x74,0x00,0x14,0x65,0xd9,0xab}}},
    {130, {{0x00,0x12,0x74,0x00,0x14,0x6f,0x11,0x78}}},
    {131, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xc0,0x6e}}},
    {132, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xee,0x36}}},
    {133, {{0x00,0x12,0x74,0x00,0x13,0xea,0x27,0xea}}},
    {135, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xf3,0x57}}},
    {136, {{0x00,0x12,0x74,0x00,0x14,0x6f,0x03,0x0a}}},
    {137, {{0x00,0x12,0x74,0x00,0x14,0x6e,0xf5,0xc9}}},
    {138, {{0x00,0x12,0x74,0x00,0x12,0xe6,0x8d,0x14}}},
#elif IN_MOTES
  /* NXP */
  /* RICH USB sticks
   *
   * 111, 00:15:8D:00:00:57:64:68 => 01
   * 112, 00:15:8D:00:00:57:64:6D => 02
   * 113, 00:15:8D:00:00:57:64:77 => 03
   * 114, 00:15:8D:00:00:57:64:85 => 04
   * 115, 00:15:8D:00:00:57:64:63 => 05
   * 116, 00:15:8D:00:00:57:64:96 => 06
   * 117, 00:15:8D:00:00:57:64:5C => 07
   * 118, 00:15:8D:00:00:57:64:94 => 08
   * 119, 00:15:8D:00:00:57:64:5D => 09
   * 120, 00:15:8D:00:00:57:64:5E => 10
   *
   * 016, 00:15:8D:00:00:36:01:88 => 16
   * 017, 00:15:8D:00:00:36:01:61 => 17
   * 018, 00:15:8D:00:00:36:01:4E => 18
   * 019, 00:15:8D:00:00:36:01:8A => 19
   * 020, 00:15:8D:00:00:36:04:DB => 20
   *
   * 161,
   * 162, => 22
   * 163, => 23
   * 164, => 24
   * 165, => 25
   * */
#elif IN_NESTESTBED

    {  1, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x68}}},
    {  2, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x6d}}},
    {  3, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x77}}},
    {  4, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x85}}},
    {  5, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x63}}},
    {  6, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x96}}},
    {  7, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x5c}}},
    {  8, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x94}}},
    {  9, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x5d}}},
    { 10, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x5e}}},
    { 11, {{0x00,0x15,0x8d,0x00,0x00,0x4d,0x73,0x48}}},
    { 12, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x8e}}},
    { 13, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x04,0xf1}}},
    { 14, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x5b}}},
    { 15, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x41}}},
    { 16, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x88}}},
    { 17, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x61}}},
    { 18, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x4e}}},
    { 19, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x01,0x8a}}},
    { 20, {{0x00,0x15,0x8d,0x00,0x00,0x36,0x04,0xdb}}},
    { 21, {{0x00,0x15,0x8d,0x00,0x00,0x4d,0x73,0x44}}},
    { 22, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0xa5}}},
    { 23, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0xb3}}},
    { 24, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0xa3}}},
    { 25, {{0x00,0x15,0x8d,0x00,0x00,0x57,0x64,0x5f}}},

#endif
  { 0, { { 0 } } }
};

uint16_t
nodex_index_map(uint16_t index)
{
#if NODE_INDEX_SUFFLE
  return (index * NODE_INDEX_SUFFLE_MULTIPLICATOR) % NODE_INDEX_SUFFLE_MODULUS;
#else
  return index;
#endif
}

/* Returns the node's node-id */
uint16_t
get_node_id()
{
#if CONTIKI_TARGET_Z1 || CONTIKI_TARGET_JN5168
  extern unsigned char node_mac[8];
  return node_id_from_linkaddr((const linkaddr_t *)node_mac);
#else
  extern unsigned char ds2411_id[8];
  return node_id_from_linkaddr((const linkaddr_t *)&ds2411_id);
#endif
}
/* Build a global link-layer address from an IPv6 based on its UUID64 */
static void
lladdr_from_ipaddr_uuid(uip_lladdr_t *lladdr, const uip_ipaddr_t *ipaddr)
{
#if (UIP_LLADDR_LEN == 8)
  memcpy(lladdr, ipaddr->u8 + 8, UIP_LLADDR_LEN);
  lladdr->addr[0] ^= 0x02;
#else
#error orpl.c supports only EUI-64 identifiers
#endif
}
/* Returns a node-index from a node's linkaddr */
uint16_t
node_index_from_linkaddr(const linkaddr_t *addr)
{
#if IN_COOJA
  if(addr == NULL) {
    return 0xffff;
  } else {
    return nodex_index_map(node_id_from_linkaddr(addr)-1);
  }
#else /* IN_COOJA */
  if(addr == NULL) {
    return 0xffff;
  }
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    /* Assume network-wide unique 16-bit MAC addresses */
    if(curr->mac.u8[6] == addr->u8[6] && curr->mac.u8[7] == addr->u8[7]) {
      return nodex_index_map(curr - id_mac_list);
    }
    curr++;
  }
  return 0xffff;
#endif /* IN_COOJA */
}
/* Returns a node-id from a node's linkaddr */
uint16_t
node_id_from_linkaddr(const linkaddr_t *addr)
{
#if IN_COOJA
  if(addr == NULL) {
    return 0;
  } else {
    return addr->u8[7];
  }
#else /* IN_COOJA */
  if(addr == NULL) {
    return 0;
  }
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    /* Assume network-wide unique 16-bit MAC addresses */
    if(curr->mac.u8[6] == addr->u8[6] && curr->mac.u8[7] == addr->u8[7]) {
      return curr->id;
    }
    curr++;
  }
  return 0;
#endif /* IN_COOJA */
}
/* Returns a node-id from a node's IPv6 address */
uint16_t
node_id_from_ipaddr(const uip_ipaddr_t *addr)
{
  uip_lladdr_t lladdr;
  lladdr_from_ipaddr_uuid(&lladdr, addr);
  return node_id_from_linkaddr((const linkaddr_t *)&lladdr);
}
/* Returns a node-index from a node-id */
uint16_t
get_node_index_from_id(uint16_t id)
{
#if IN_COOJA
  //return nodex_index_map(id - 1);
  return nodex_index_map(id);
#else
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    if(curr->id == id) {
      return nodex_index_map(curr - id_mac_list);
    }
    curr++;
  }
  return 0xffff;
#endif
}
/* Sets an IPv6 from a node-id */
void
set_ipaddr_from_id(uip_ipaddr_t *ipaddr, uint16_t id)
{
  linkaddr_t lladdr;
  memcpy(ipaddr, &prefix, 8);
  set_linkaddr_from_id(&lladdr, id);
  uip_ds6_set_addr_iid(ipaddr, (uip_lladdr_t *)&lladdr);
}
/* Sets an linkaddr from a link-layer address */
/* Sets a linkaddr from a node-id */
void
set_linkaddr_from_id(linkaddr_t *lladdr, uint16_t id)
{
#if IN_COOJA
  lladdr->u8[0] = 0x00;
  lladdr->u8[1] = 0x12;
  lladdr->u8[2] = 0x74;
  lladdr->u8[3] = id;
  lladdr->u8[4] = 0x00;
  lladdr->u8[5] = id;
  lladdr->u8[6] = id;
  lladdr->u8[7] = id;
#else
  if(id == 0 || lladdr == NULL) {
    return;
  }
  const struct id_mac *curr = id_mac_list;
  while(curr->id != 0) {
    if(curr->id == id) {
      linkaddr_copy(lladdr, &curr->mac);
      return;
    }
    curr++;
  }
#endif
}
/* Initializes global IPv6 and creates DODAG */
int
deployment_init(uip_ipaddr_t *ipaddr, uip_ipaddr_t *br_prefix, int root_id)
{
  rpl_dag_t *dag;
  
  node_id_restore();
  node_index = get_node_index_from_id(node_id);

  if(node_id == 0) {
    NETSTACK_RDC.off(0);
    NETSTACK_MAC.off(0);
    return 0;
  }

#if CONTIKI_TARGET_SKY || CONTIKI_TARGET_Z1
  NETSTACK_RADIO_set_txpower(RF_POWER);
  NETSTACK_RADIO_set_cca_threshold(RSSI_THR);
#endif
  
  if(!br_prefix) {
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  } else {
    memcpy(&prefix, br_prefix, 16);
  }
  set_ipaddr_from_id(ipaddr, node_id);
  uip_ds6_addr_add(ipaddr, 0, ADDR_AUTOCONF);

  if(WITH_RPL && node_id == root_id) {
    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    rpl_set_prefix(dag, &prefix, 64);
#if WITH_TSCH
    tsch_is_coordinator = 1;
#endif
    /* If an RDC layer is used, turn it off (i.e. keep the radio on at the root) */
    NETSTACK_RDC.off(1);
  }

#if WITH_LOG
  log_start();
#endif /* WITH_LOG */

  NETSTACK_MAC.on();

  return 1;
}

#endif /* WITH_DEPLOYMENT */

