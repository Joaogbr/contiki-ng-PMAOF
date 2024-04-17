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

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

/* Blipcare BP meter */
/* From: IoT Network Traffic Classification Using Machine Learning Algorithms: An Experimental Analysis */
/* Avg payload length: 71.44 B, avg packet length: 125.35 B */
/* Avg tx rate: 996.83 B/min, avg sleep interval: 7.76 s */
// 70 B payload -> 126 B IPv6 packet
#define BUFSIZE 70
#define SEND_INTERVAL		  (uint16_t) (1 * CLOCK_SECOND)

static char buf[BUFSIZE-12] = { [0 ... (BUFSIZE-13)] = '@' };
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
  char str[datalen];
  char *ptr;
  uint32_t seqnum;
  strcpy(str, (char *) data);
  seqnum = strtoul(&str[6], &ptr, 10);
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
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[BUFSIZE+1];
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
      snprintf(str, sizeof(str), "%s, %10" PRIu32 "", buf, tx_count);
      LOG_INFO("app generate packet seqnum=%" PRIu32 " node_id=%u\n", tx_count, node_id);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
    } else {
      LOG_INFO("Not reachable yet\n");
      if(tx_count > 0) {
        missed_tx_count++;
      }
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - SEND_INTERVAL / 16 + (random_rand() % (SEND_INTERVAL / 8)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
