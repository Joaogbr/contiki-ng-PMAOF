#include "messages.h"
#include "sys/log.h"
#include <stdlib.h>
#include "sys/clock.h"


#include "contiki-net.h"


#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

message_type_t GetMessageType (const uint8_t *data, uint16_t datalen){
    packet_t *packet = (packet_t *) data;
    return packet->msg_type - '0';
}

void print_own_ipv6(void) {
  uip_ds6_addr_t *lladdr;
  memcpy(&uip_lladdr.addr, &linkaddr_node_addr, sizeof(uip_lladdr.addr));
  lladdr = uip_ds6_get_link_local(-1);
  LOG_INFO("My IPv6 address: ");
  LOG_INFO_6ADDR(lladdr != NULL ? &lladdr->ipaddr : NULL);
  LOG_INFO_("\n");
}

uip_ipaddr_t get_own_ip(void) {
  uip_ds6_addr_t *lladdr;
  lladdr = uip_ds6_get_link_local(-1);
  return lladdr->ipaddr;
}