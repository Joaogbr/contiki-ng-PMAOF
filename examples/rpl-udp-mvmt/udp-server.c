/*
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

#include "contiki.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-classic/rpl.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Coordinator Node ID */
#if CONTIKI_TARGET_IOTLAB
/* for FIT IoT lab */
#define COORDINATOR_ID 42088
#else
/* for simulations */
#define COORDINATOR_ID 1
#endif

#define WITH_SERVER_REPLY  0
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

static struct simple_udp_connection udp_conn;
uint32_t seqnumtx;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  char str[10];
  char *ptr;
  uint32_t seqnumrx;
  strcpy(str, (char *) &data[datalen-10]);
  seqnumrx = strtoul(str, &ptr, 10);
  LOG_INFO("Received request '%.*s'\n", datalen, (char *) data);
  LOG_INFO("app receive packet seqnum=%" PRIu32 " from=", seqnumrx);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
#if WITH_SERVER_REPLY
  /* send back a hello string to the client as a reply */
  char rep[18];
  seqnumtx++;
  snprintf(rep, sizeof(rep), "hello, %10" PRIu32 "", seqnumrx);
  LOG_INFO("Sending response.\n");
  LOG_INFO("app generate packet seqnum=%" PRIu32 " node_id=%u\n", seqnumtx, node_id);
  simple_udp_sendto(&udp_conn, rep, strlen(rep), sender_addr);
#endif /* WITH_SERVER_REPLY */
}
/*---------------------------------------------------------------------------*/
#if !MAC_CONF_WITH_TSCH
void
mvmt_rpl_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new)
{
  if(new != NULL) {
    LOG_INFO("rpl callback: new parent lladdr -> ");
    LOG_INFO_LLADDR(rpl_get_parent_lladdr(new));
    LOG_INFO_("\n");
  } else if(!NETSTACK_ROUTING.node_is_reachable()) {
    LOG_INFO("rpl callback: node has left the network\n");
  }
}
#endif
/*---------------------------------------------------------------------------*/
#if NBR_TABLE_GC_GET_WORST==rpl_nbr_gc_get_worst_path
const linkaddr_t *
rpl_nbr_gc_get_worst_path(const linkaddr_t *lladdr1, const linkaddr_t *lladdr2)
{
  rpl_parent_t *p1 = rpl_get_parent((uip_lladdr_t *)lladdr1);
  rpl_parent_t *p2 = rpl_get_parent((uip_lladdr_t *)lladdr2);
  if(p1 != NULL && p2 != NULL && p1->dag != NULL) {
    rpl_instance_t *instance = p1->dag->instance;
    if(instance != NULL && instance->of != NULL &&
       instance->of->parent_path_cost != NULL) {
      return instance->of->parent_path_cost(p2) > instance->of->parent_path_cost(p1) ? lladdr2 : lladdr1;
    }
  }
  return rpl_rank_via_parent(p2) > rpl_rank_via_parent(p1) ? lladdr2 : lladdr1;
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  //static struct etimer periodic_timer;

  PROCESS_BEGIN();

  //etimer_set(&periodic_timer, APP_WARM_UP_PERIOD_SEC * CLOCK_SECOND);

  //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

  /* Initialize DAG root */
  LOG_INFO("set as root\n");
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
