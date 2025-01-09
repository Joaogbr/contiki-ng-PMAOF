#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "messages.h"
#include "node-id.h"
#include "sys/clock.h"
#include "contiki.h"
#include "contiki-net.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "sys/energest.h"
#include "sys/stack-check.h"
#include "dev/watchdog.h"

#include "net/queuebuf.h"
#include "net/app-layer/coap/coap-engine.h"
#include "net/app-layer/snmp/snmp.h"
#include "services/rpl-border-router/rpl-border-router.h"
#include "services/orchestra/orchestra.h"
#include "services/shell/serial-shell.h"
#include "services/simple-energest/simple-energest.h"
#include "services/tsch-cs/tsch-cs.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
// #define LOG_CONF_LEVEL_RPL LOG_LEVEL_INFO


#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define LOCATION_INTERVAL		  (1 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

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
  LOG_INFO("Received packet ");
  LOG_INFO_6ADDR(receiver_addr);
  LOG_INFO_(" '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  PROCESS_BEGIN();
  LOG_INFO("SOU UM VEICULO NORMAL\n");
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);
  print_own_ipv6();
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
