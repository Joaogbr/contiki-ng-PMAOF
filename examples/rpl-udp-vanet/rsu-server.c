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

#include "contiki-net.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uip-debug.h"

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

#define BUFSIZE 70
#define MAX_TX_COUNT 1200
#define SEND_INTERVAL		  (uint16_t) (1 * (CLOCK_SECOND >> 7))
#define SEND_INTERVAL_DELAY		  (uint16_t) (1 * (CLOCK_SECOND))

static char buf[BUFSIZE-12] = { [0 ... (BUFSIZE-13)] = '@' };

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
PROCESS_THREAD(udp_server_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer delay_timer;
  static char str[BUFSIZE+1];
  static uint32_t tx_count;
  // static uint32_t missed_tx_count;

  PROCESS_BEGIN();

  /* Initialize DAG root */
  LOG_INFO("set as root\n");
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  uip_ds6_addr_t *lladdr;
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  lladdr = uip_ds6_get_link_local(-1);
  uip_ipaddr_t own_ip = lladdr->ipaddr;
  own_ip.u16[0] = UIP_HTONS(0xfd00);  // Define os primeiros 16 bits para fd00
  LOG_INFO("VANET My IPv6 address: ");
  LOG_INFO_6ADDR(&own_ip);
  LOG_INFO_("\n");

  static uip_ipaddr_t my_ipv6_addr;

  // Definir o endereço IPv6 manualmente
// Definir o endereço IPv6 manualmente
  my_ipv6_addr.u16[0] = UIP_HTONS(0xfd00); // fd00::
  my_ipv6_addr.u16[1] = UIP_HTONS(0x0000); // ::0
  my_ipv6_addr.u16[2] = UIP_HTONS(0x0000); // ::0
  my_ipv6_addr.u16[3] = UIP_HTONS(0x0000); // ::0
  my_ipv6_addr.u16[4] = UIP_HTONS(0x0256); // ::256
  my_ipv6_addr.u16[5] = UIP_HTONS(0x0056); // ::256:56
  my_ipv6_addr.u16[6] = UIP_HTONS(0x0056); // ::256:56:56
  my_ipv6_addr.u16[7] = UIP_HTONS(0x0056); // ::256:56:56:56

  LOG_INFO("VANET2 My IPv6 address: ");
  LOG_INFO_6ADDR(&my_ipv6_addr);
  LOG_INFO_("\n");

  static uip_sr_node_t *node;
  etimer_set(&periodic_timer, APP_WARM_UP_PERIOD_SEC * CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    node = uip_sr_node_head();
    while(node != NULL && tx_count <= MAX_TX_COUNT) {
      static uip_ipaddr_t node_addr;

      NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);
      if (uip_ipaddr_cmp(&node_addr, &my_ipv6_addr)) {
        node = uip_sr_node_next(node);
        continue;
      }

      LOG_INFO("Sending request %"PRIu32" to ", tx_count);
      LOG_INFO_6ADDR(&node_addr);
      LOG_INFO_("\n");
      tx_count++;
      snprintf(str, sizeof(str), "%s, %10" PRIu32 "", buf, tx_count);
      LOG_INFO("app generate packet seqnum=%" PRIu32 " node_id=%u\n", tx_count, node_id);
      simple_udp_sendto(&udp_conn, str, strlen(str), &node_addr);
      node = uip_sr_node_next(node);
      etimer_set(&delay_timer, SEND_INTERVAL_DELAY);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&delay_timer));
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL);
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
