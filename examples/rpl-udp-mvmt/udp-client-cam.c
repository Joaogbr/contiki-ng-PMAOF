#include "contiki.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-classic/rpl.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

/* Custom parameters for smart cameras */
/* From: IoT Traffic: Modeling and Measurement Experiments */
/* Avg packet length: 1000 B */
/* Avg tx period: 0.05 s */
// 944 B payload -> 1000 B IPv6 packet
#define BUFSIZE 944
#define SEND_INTERVAL		  (uint8_t) (CLOCK_SECOND / 20)

static char buf[BUFSIZE+1] = { [0 ... (BUFSIZE-13)] = '@', [BUFSIZE-12] = ',', [(BUFSIZE-11) ... (BUFSIZE)] = ' ' };
static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
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
  uint32_t seqnum;
  memcpy(str, (char *) &data[datalen-10], sizeof(str));
  seqnum = strtoul(str, &ptr, 10);
  LOG_INFO("Received response '%.*s'\n", datalen, (char *) data);
  LOG_INFO("app receive packet seqnum=%" PRIu32 " from=", seqnum);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
  rx_count++;
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
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, APP_WARM_UP_PERIOD_SEC * CLOCK_SECOND
    + random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() &&
        NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

      /* Print statistics every 10th TX */
      if(tx_count % 10 == 0) {
        LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                 tx_count, rx_count, missed_tx_count);
      }

      /* Send to DAG root */
      LOG_INFO("Sending request %"PRIu32" to ", tx_count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      tx_count++;
      snprintf(&buf[BUFSIZE-10], 11, "%10" PRIu32 "", tx_count);
      LOG_INFO("app generate packet seqnum=%" PRIu32 " node_id=%u\n", tx_count, node_id);
      simple_udp_sendto(&udp_conn, buf, strlen(buf), &dest_ipaddr);
    } else {
      LOG_INFO("Not reachable yet\n");
      if(tx_count > 0) {
        missed_tx_count++;
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND/128 + (random_rand() % (CLOCK_SECOND/64)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
