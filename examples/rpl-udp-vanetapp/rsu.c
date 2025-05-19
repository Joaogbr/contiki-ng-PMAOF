#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "messages.h"
#include "sys/log.h"
#include "node-id.h"
#include "contiki-net.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uip-debug.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

uip_ipaddr_t own_ip;

static struct simple_udp_connection udp_conn;
static process_event_t accident_message_event;

/*---------------------------------------------------------------------------*/

PROCESS(send_accident_msg, "Send accident");
PROCESS(udp_server_process, "UDP server");


AUTOSTART_PROCESSES(&send_accident_msg,&udp_server_process);
PROCESS_THREAD(send_accident_msg, ev, data)
{
  PROCESS_BEGIN();
  accident_message_event = process_alloc_event();

  static uip_sr_node_t *node;

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == accident_message_event);
    node = uip_sr_node_head();
    while(node != NULL) {
      static uip_ipaddr_t node_addr;
      NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);
      if (uip_ipaddr_cmp(&node_addr, &own_ip)) {
        node = uip_sr_node_next(node);
        continue;
      }
      int node_id_rcv, msg_type, num_seq, x, y, z;

      sscanf((const char *)data, "Node Id: %d num_seq: %d msg type: %d x: %d y: %d z: %d", &node_id_rcv, &num_seq, &msg_type, &x, &y, &z);
      static char str[64];
      snprintf(str, sizeof(str), "num_seq: %d msg type: %d", num_seq, ACCIDENT_REPORT_DISSEMINATION);
      LOG_INFO("Sent packet ");
      LOG_INFO_6ADDR(&own_ip);
      LOG_INFO_(" '%.*s' to ", (int)strlen(str), (char *)str);
      LOG_INFO_6ADDR(&node_addr);
      LOG_INFO_("\n");
      simple_udp_sendto(&udp_conn, str, strlen(str) + 1, &node_addr);
      node = uip_sr_node_next(node);
    }
  }
  PROCESS_END();
}

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
  own_ip = *receiver_addr;
  const uip_ipaddr_t receiver_addr_copy = *receiver_addr;
  LOG_INFO("Received packet ");
  LOG_INFO_6ADDR(&receiver_addr_copy);
  LOG_INFO_(" '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
  int node_id_rcv, num_seq, msg_type, x, y, z;
  sscanf((const char *)data, "Node Id: %d num_seq: %d msg type: %d x: %d y: %d z: %d",&node_id_rcv,  &num_seq, &msg_type, &x, &y, &z);
  switch(msg_type) {
    case ACCIDENT_REPORT:
      process_post(&send_accident_msg, accident_message_event, (void *)data);
      // send_accident_report_to_rsus(datalen, (char *) data);
    break;
    case ACCIDENT_REPORT_DISSEMINATION:
      process_post(&send_accident_msg, accident_message_event, (void *)data);
    break;
  }
  
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();
  own_ip = get_own_ip();
  /* Set ipaddr with DODAG ID, so we get the prefix */
  static uip_ipaddr_t ipaddr;
  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();
  NETSTACK_ROUTING.get_root_ipaddr(&ipaddr);
  LOG_INFO("Node ID %d has IP ", node_id);
  LOG_INFO_6ADDR(&ipaddr);
  LOG_INFO_("\n");
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  print_own_ipv6();
  PROCESS_END();
}
