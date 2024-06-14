/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 *
 * Authors: Simon Duquennoy <simonduq@sics.se>
 */

#ifndef LINK_STATS_H_
#define LINK_STATS_H_

#include "lib/fixmath.h"
#include "net/linkaddr.h"

/* Statistics with no update in FRESHNESS_EXPIRATION_TIMEOUT is not fresh */
#define FRESHNESS_EXPIRATION_TIME       (8 * 60 * (clock_time_t)CLOCK_SECOND) //(10 * 60 * (clock_time_t)CLOCK_SECOND)
/* Half time for the freshness counter */
#define FRESHNESS_HALF_LIFE             (15 * 60 * (clock_time_t)CLOCK_SECOND)
/* Statistics are fresh if the freshness counter is FRESHNESS_TARGET or more */
#define FRESHNESS_TARGET                 4
/* Maximum value for the freshness counter */
#define FRESHNESS_MAX                   16

/* ETX fixed point divisor. 128 is the value used by RPL (RFC 6551 and RFC 6719) */
#ifdef LINK_STATS_CONF_ETX_DIVISOR
#define LINK_STATS_ETX_DIVISOR LINK_STATS_CONF_ETX_DIVISOR
#else /* LINK_STATS_CONF_ETX_DIVISOR */
#define LINK_STATS_ETX_DIVISOR                   128
#endif /* LINK_STATS_CONF_ETX_DIVISOR */

/* Option to infer the initial ETX from the RSSI of previously received packets. */
#ifdef LINK_STATS_CONF_INIT_ETX_FROM_RSSI
#define LINK_STATS_INIT_ETX_FROM_RSSI LINK_STATS_CONF_INIT_ETX_FROM_RSSI
#else /* LINK_STATS_CONF_INIT_ETX_FROM_RSSI */
#define LINK_STATS_INIT_ETX_FROM_RSSI              1
#endif /* LINK_STATS_CONF_INIT_ETX_FROM_RSSI */

/* Option to use packet and ACK count for ETX estimation, instead of EWMA */
#ifdef LINK_STATS_CONF_ETX_FROM_PACKET_COUNT
#define LINK_STATS_ETX_FROM_PACKET_COUNT LINK_STATS_CONF_ETX_FROM_PACKET_COUNT
#else /* LINK_STATS_CONF_ETX_FROM_PACKET_COUNT */
#define LINK_STATS_ETX_FROM_PACKET_COUNT           0
#endif /* LINK_STATS_ETX_FROM_PACKET_COUNT */

/* Store and periodically print packet counters? */
#ifdef LINK_STATS_CONF_PACKET_COUNTERS
#define LINK_STATS_PACKET_COUNTERS LINK_STATS_CONF_PACKET_COUNTERS
#else /* LINK_STATS_CONF_PACKET_COUNTERS */
#define LINK_STATS_PACKET_COUNTERS           0
#endif /* LINK_STATS_PACKET_COUNTERS */

/* Maximal initial ETX value when guessed from RSSI */
#ifdef LINK_STATS_CONF_ETX_INIT_MAX
#define LINK_STATS_ETX_INIT_MAX LINK_STATS_CONF_ETX_INIT_MAX
#else /* LINK_STATS_CONF_ETX_INIT_MAX */
#define LINK_STATS_ETX_INIT_MAX              3
#endif /* LINK_STATS_ETX_INIT_MAX */

/* "Good" RSSI value when ETX is guessed from RSSI */
#ifdef LINK_STATS_CONF_RSSI_HIGH
#define LINK_STATS_RSSI_HIGH LINK_STATS_CONF_RSSI_HIGH
#else /* LINK_STATS_CONF_RSSI_HIGH */
#define LINK_STATS_RSSI_HIGH               -60
#endif /* LINK_STATS_RSSI_HIGH */

/* "Bad" RSSI value when ETX is guessed from RSSI */
#ifdef LINK_STATS_CONF_RSSI_LOW
#define LINK_STATS_RSSI_LOW LINK_STATS_CONF_RSSI_LOW
#else /* LINK_STATS_CONF_RSSI_LOW */
#define LINK_STATS_RSSI_LOW                -90
#endif /* LINK_STATS_RSSI_LOW */

/* Determines how many RSSI values are recorded */
#ifdef LINK_STATS_CONF_RSSI_ARR_LEN
#define LINK_STATS_RSSI_ARR_LEN LINK_STATS_CONF_RSSI_ARR_LEN
#else /* LINK_STATS_CONF_RSSI_ARR_LEN */
#define LINK_STATS_RSSI_ARR_LEN                3
#endif /* LINK_STATS_RSSI_ARR_LEN */

/* Determines how many RSSI values are sufficient */
#ifdef LINK_STATS_CONF_MIN_RSSI_COUNT
#define LINK_STATS_MIN_RSSI_COUNT LINK_STATS_CONF_MIN_RSSI_COUNT
#else /* LINK_STATS_CONF_MIN_RSSI_COUNT */
#define LINK_STATS_MIN_RSSI_COUNT                2
#endif /* LINK_STATS_MIN_RSSI_COUNT */

/* Determines how RSSI values are recorded */
#ifdef LINK_STATS_CONF_RSSI_WITH_EMANEXT
#define LINK_STATS_RSSI_WITH_EMANEXT LINK_STATS_CONF_RSSI_WITH_EMANEXT
#else /* LINK_STATS_CONF_RSSI_WITH_EMANEXT */
#define LINK_STATS_RSSI_WITH_EMANEXT                0
#endif /* LINK_STATS_RSSI_WITH_EMANEXT */

/* Determines how RSSI values are recorded */
#ifdef LINK_STATS_CONF_ETX_WITH_EMANEXT
#define LINK_STATS_ETX_WITH_EMANEXT LINK_STATS_CONF_ETX_WITH_EMANEXT
#else /* LINK_STATS_CONF_ETX_WITH_EMANEXT */
#define LINK_STATS_ETX_WITH_EMANEXT                0
#endif /* LINK_STATS_ETX_WITH_EMANEXT */

/* Special value that signal the RSSI is not initialized */
#define LINK_STATS_RSSI_UNKNOWN 0x7fff

/* Determines how many failed probes are tolerated */
#ifdef LINK_STATS_CONF_FAILED_PROBES_MAX_NUM
#define LINK_STATS_FAILED_PROBES_MAX_NUM LINK_STATS_CONF_FAILED_PROBES_MAX_NUM
#else /* LINK_STATS_CONF_FAILED_PROBES_MAX_NUM */
#define LINK_STATS_FAILED_PROBES_MAX_NUM                2
#endif /* LINK_STATS_FAILED_PROBES_MAX_NUM */

typedef uint16_t link_packet_stat_t;

struct link_packet_counter {
  /* total attempts to transmit unicast packets */
  link_packet_stat_t num_packets_tx;
  /* total ACKs for unicast packets */
  link_packet_stat_t num_packets_acked;
  /* total number of unicast and broadcast packets received */
  link_packet_stat_t num_packets_rx;
  /* total number of packets dropped before transmission due to insufficient memory */
  link_packet_stat_t num_queue_drops;
};


/* All statistics of a given link */
struct link_stats {
  clock_time_t last_tx_time;  /* Last Tx timestamp */
  clock_time_t last_rx_time;  /* Last Rx timestamp */
  clock_time_t rx_time[LINK_STATS_RSSI_ARR_LEN];  /* Last Rx timestamps */
  clock_time_t last_probe_time;  /* Last Probe (DIO/DIS) timestamp */
  //clock_time_t last_lm_time; /* Last link metric timestamp */
  uint16_t etx;               /* ETX using ETX_DIVISOR as fixed point divisor. Zero if not yet measured. */
  fix16_t last_rssi; /* Latest RSSI (received signal strength) value. LINK_STATS_RSSI_UNKNOWN if not yet measured. */
  fix16_t rssi[LINK_STATS_RSSI_ARR_LEN]; /* Latest RSSI (received signal strength) values. LINK_STATS_RSSI_UNKNOWN if not yet measured. */
  uint8_t freshness;          /* Freshness of the statistics. Zero if no packets sent yet. */
  uint8_t failed_probes;          /* Number of lost probes. */
  uint8_t link_stats_metric_updated; /* Set when values are updated */
  fix16_t last_link_metric; /* OF calculated link metric */
  //fix16_t last_cf; /* Correction Factor */
#if LINK_STATS_ETX_FROM_PACKET_COUNT
  uint8_t tx_count;           /* Tx count, used for ETX calculation */
  uint8_t ack_count;          /* ACK count, used for ETX calculation */
#endif /* LINK_STATS_ETX_FROM_PACKET_COUNT */

#if LINK_STATS_PACKET_COUNTERS
  struct link_packet_counter cnt_current; /* packets in the current period */
  struct link_packet_counter cnt_total;   /* packets in total */
#endif
};

/* Returns the neighbor's link statistics */
const struct link_stats *link_stats_from_lladdr(const linkaddr_t *lladdr);
/* Returns the address of the neighbor */
const linkaddr_t *link_stats_get_lladdr(const struct link_stats *);
/* Are the transmissions fresh? */
int link_stats_tx_fresh(const struct link_stats *stats, clock_time_t exp_time);
/* Are the receptions fresh? */
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
int link_stats_rx_fresh(const struct link_stats *stats, clock_time_t exp_time);
#endif
/* Was the link probed recently? */
int link_stats_recent_probe(const struct link_stats *stats, clock_time_t exp_time);
/* Returns number of RSSI measurements */
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
uint8_t link_stats_get_rssi_count(const struct link_stats *stats, int fresh_only);
#endif
/* Resets link-stats module */
void link_stats_reset(void);
/* Initializes link-stats module */
void link_stats_init(void);
/* Packet sent callback. Updates statistics for transmissions on a given link */
void link_stats_packet_sent(const linkaddr_t *lladdr, int status, int numtx);
/* Packet input callback. Updates statistics for receptions on a given link */
void link_stats_input_callback(const linkaddr_t *lladdr);
/* Updates Objective Function result for a given link */
void link_stats_metric_update_callback(const linkaddr_t *lladdr, fix16_t link_metric/*, clock_time_t lm_time*/);
/* Updates last probing time for a given link */
void link_stats_probe_callback(const linkaddr_t *lladdr, clock_time_t probe_time);

#endif /* LINK_STATS_H_ */
