#include <stdint.h> // For C


#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>
#include "node-id.h"
#include "sys/clock.h"
#include "contiki.h"
#include "contiki-net.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "sys/energest.h"
#include "sys/stack-check.h"
#include "dev/watchdog.h"

#define MAX_MSG_SIZE  100


typedef enum {
  ACCIDENT_REPORT = 1,
  ACCIDENT_REPORT_DISSEMINATION,
} message_type_t;

typedef struct {
    const uint8_t msg_type_header[10]; 
    uint8_t msg_type;
    const uint8_t *data;
    uint16_t datalen;
} packet_t;

// Node: %d msg type: %d x: %d y: %d z: %d
// "Node: %d num_seq: %d msg type: %d x: %d y: %d z: %d"
typedef struct {
  char node_header[6];
  uint8_t sender_node_id;
  char num_seq_header[10];
  uint8_t num_seq;
  char msg_type_header[11]; 
  uint8_t msg_type;
  char x_header[5];
  unsigned short x;
  char y_header[5];
  unsigned short y;
  char z_header[4];
  unsigned short z;
} accident_report_packet_t;

message_type_t GetMessageType (const uint8_t *data, uint16_t datalen);

void print_own_ipv6(void);

uip_ipaddr_t get_own_ip(void);