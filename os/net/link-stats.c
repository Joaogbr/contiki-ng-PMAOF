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
 *          Atis Elsts <atis.elsts@edi.lv>
 */

#include "contiki.h"
#include "sys/clock.h"
#include "net/packetbuf.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"
#include <stdio.h>
#include <math.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Link Stats"
#define LOG_LEVEL LOG_LEVEL_LS

/* Maximum value for the Tx count counter */
#define TX_COUNT_MAX                    32

/* EWMA (exponential moving average) used to maintain statistics over time */
#define EWMA_SCALE                     100
#define EWMA_ALPHA                      10
#define EWMA_BOOTSTRAP_ALPHA            25
#define EMA_TAU                         10 /* Seconds */

/* ETX fixed point divisor. 128 is the value used by RPL (RFC 6551 and RFC 6719) */
#define ETX_DIVISOR                     LINK_STATS_ETX_DIVISOR
/* In case of no-ACK, add ETX_NOACK_PENALTY to the real Tx count, as a penalty */
#define ETX_NOACK_PENALTY               12
/* Initial ETX value */
#define ETX_DEFAULT                      2

#define RSSI_DIFF (LINK_STATS_RSSI_HIGH - LINK_STATS_RSSI_LOW)

#define STATIC_DET_TIME_THRESH           (7 * (clock_time_t)CLOCK_SECOND)
#define STATIC_DET_RSSI_THRESH           0.5f

/* Generate error on incorrect link stats configuration values */
#if RSSI_DIFF <= 0
#error "RSSI_HIGH must be greater then RSSI_LOW"
#endif

/* Generate error if the initial ETX calculation would overflow uint16_t */
#if ETX_DIVISOR * RSSI_DIFF >= 0x10000
#error "RSSI math overflow"
#endif

/* Per-neighbor link statistics table */
NBR_TABLE(struct link_stats, link_stats);

/* Called at a period of FRESHNESS_HALF_LIFE */
struct ctimer periodic_timer;

/*---------------------------------------------------------------------------*/
/* Returns the neighbor's link stats */
const struct link_stats *
link_stats_from_lladdr(const linkaddr_t *lladdr)
{
  return nbr_table_get_from_lladdr(link_stats, lladdr);
}
/*---------------------------------------------------------------------------*/
/* Returns the neighbor's address given a link stats item */
const linkaddr_t *
link_stats_get_lladdr(const struct link_stats *stat)
{
  return nbr_table_get_lladdr(link_stats, stat);
}
/*---------------------------------------------------------------------------*/
/* Are the transmissions fresh? */
int
link_stats_tx_fresh(const struct link_stats *stats, clock_time_t exp_time)
{
  return (stats != NULL)
      && clock_time() - stats->last_tx_time < exp_time
      && stats->freshness >= FRESHNESS_TARGET;
}
/*---------------------------------------------------------------------------*/
#if RPL_DAG_MC == RPL_DAG_MC_SSV
/* Are the receptions fresh? */
int
link_stats_rx_fresh(const struct link_stats *stats, clock_time_t exp_time)
{
  return (stats != NULL)
      && clock_time() - stats->rx_time[0] < exp_time;
}
#endif
/*---------------------------------------------------------------------------*/
/* Was the Link probed recently? */
int
link_stats_recent_probe(const struct link_stats *stats, clock_time_t exp_time)
{
  return (stats != NULL)
      && clock_time() - stats->last_probe_time < exp_time;
}
/*---------------------------------------------------------------------------*/
#if RPL_DAG_MC == RPL_DAG_MC_SSV
/* Are the receptions fresh? */
uint8_t
link_stats_get_rssi_count(const fix16_t rssi_arr[], const clock_time_t rx_time_arr[], int fresh_only)
{
  /* type = 0 -> get no. of measurements
   * type = 1 -> get no. of fresh measurements
   */
  uint8_t count = LINK_STATS_RSSI_ARR_LEN;
  while((count > 0) && (rssi_arr[count-1] == fix16_from_int(LINK_STATS_RSSI_UNKNOWN))) {
    count--;
  }
  if(fresh_only) {
    clock_time_t clock_now = clock_time();
    for(uint8_t i = count; i > 0; i--) {
      /* Freshness windows are proportional to the order of the samples. */
      if((clock_now - rx_time_arr[i-1]) >= (FRESHNESS_EXPIRATION_TIME * i)) {
        count--;
      }
    }
  }
  return count;
}
#endif
/*---------------------------------------------------------------------------*/
#if LINK_STATS_INIT_ETX_FROM_RSSI
/*
 * Returns initial ETX value from an RSSI value.
 *    RSSI >= RSSI_HIGH           -> use default ETX
 *    RSSI_LOW < RSSI < RSSI_HIGH -> ETX is a linear function of RSSI
 *    RSSI <= RSSI_LOW            -> use minimal initial ETX
 **/
static uint16_t
guess_etx_from_rssi(const struct link_stats *stats)
{
  if(stats != NULL) {
    if(stats->rssi[0] == fix16_from_int(LINK_STATS_RSSI_UNKNOWN)) {
      return ETX_DEFAULT * ETX_DIVISOR;
    } else {
      const int16_t rssi_delta = fix16_to_int(stats->rssi[0]) - LINK_STATS_RSSI_LOW;
      const int16_t bounded_rssi_delta = BOUND(rssi_delta, 0, RSSI_DIFF);
      /* Penalty is in the range from 0 to ETX_DIVISOR */
      const uint16_t penalty = ETX_DIVISOR * bounded_rssi_delta / RSSI_DIFF;
      /* ETX is the default ETX value + penalty */
      const uint16_t etx = ETX_DIVISOR * ETX_DEFAULT + penalty;
      return MIN(etx, LINK_STATS_ETX_INIT_MAX * ETX_DIVISOR);
    }
  }
  return 0xffff;
}
#endif /* LINK_STATS_INIT_ETX_FROM_RSSI */
/*---------------------------------------------------------------------------*/
/* Initialize rssi values from link_stats stats */
static void initialize_rssi_stats(struct link_stats *stats)
{
  for(uint8_t i = 0; i < LINK_STATS_RSSI_ARR_LEN; i++) {
    stats->rssi[i] = fix16_from_int(LINK_STATS_RSSI_UNKNOWN);
    stats->rx_time[i] = 0;
    stats->nbr_rssi[i] = fix16_from_int(LINK_STATS_RSSI_UNKNOWN);
    stats->nbr_rx_time[i] = 0;
  }
  stats->last_ssv = fix16_minimum;
  stats->last_ssr = 0;
  stats->last_rx_time = 0;
  stats->last_probe_time = 0;
  stats->link_stats_metric_updated = 0xff;
}
/*---------------------------------------------------------------------------*/
/* Packet sent callback. Updates stats for transmissions to lladdr */
void
link_stats_packet_sent(const linkaddr_t *lladdr, int status, int numtx)
{
  struct link_stats *stats;
#if !LINK_STATS_ETX_FROM_PACKET_COUNT
  uint16_t packet_etx;
  uint8_t ewma_alpha;
#endif /* !LINK_STATS_ETX_FROM_PACKET_COUNT */

  if(status != MAC_TX_OK && status != MAC_TX_NOACK && status != MAC_TX_QUEUE_FULL) {
    /* Do not penalize the ETX when collisions or transmission errors occur. */
    return;
  }

  stats = nbr_table_get_from_lladdr(link_stats, lladdr);
  if(stats == NULL) {
    /* If transmission failed, do not add the neighbor, as the neighbor might not exist anymore */
    if(status != MAC_TX_OK) {
      return;
    }

    /* Add the neighbor */
    stats = nbr_table_add_lladdr(link_stats, lladdr, NBR_TABLE_REASON_LINK_STATS, NULL);
    if(stats == NULL) {
      return; /* No space left, return */
    }
    initialize_rssi_stats(stats);
  }

  if(status == MAC_TX_QUEUE_FULL) {
#if LINK_STATS_PACKET_COUNTERS
    stats->cnt_current.num_queue_drops += 1;
#endif
    /* Do not penalize the ETX when the packet is dropped due to a full queue */
    return;
  }

  /* Update last timestamp and freshness */
  stats->last_tx_time = clock_time();
  stats->freshness = MIN(stats->freshness + numtx, FRESHNESS_MAX);

#if LINK_STATS_PACKET_COUNTERS
  /* Update paket counters */
  stats->cnt_current.num_packets_tx += numtx;
  if(status == MAC_TX_OK) {
    stats->cnt_current.num_packets_acked++;
  }
#endif

  /* Add penalty in case of no-ACK */
  if(status == MAC_TX_NOACK) {
    numtx += ETX_NOACK_PENALTY;
  }

#if LINK_STATS_ETX_FROM_PACKET_COUNT
  /* Compute ETX from packet and ACK count */
  /* Halve both counter after TX_COUNT_MAX */
  if(stats->tx_count + numtx > TX_COUNT_MAX) {
    stats->tx_count /= 2;
    stats->ack_count /= 2;
  }
  /* Update tx_count and ack_count */
  stats->tx_count += numtx;
  if(status == MAC_TX_OK) {
    stats->ack_count++;
  }
  /* Compute ETX */
  if(stats->ack_count > 0) {
    stats->etx = ((uint16_t)stats->tx_count * ETX_DIVISOR) / stats->ack_count;
  } else {
    stats->etx = (uint16_t)MAX(ETX_NOACK_PENALTY, stats->tx_count) * ETX_DIVISOR;
  }
#else /* LINK_STATS_ETX_FROM_PACKET_COUNT */
  /* Compute ETX using an EWMA */

  /* ETX used for this update */
  packet_etx = numtx * ETX_DIVISOR;
  /* ETX alpha used for this update */
  ewma_alpha = link_stats_tx_fresh(stats, FRESHNESS_EXPIRATION_TIME) ? EWMA_ALPHA : EWMA_BOOTSTRAP_ALPHA;

  if(stats->etx == 0) {
    /* Initialize ETX */
    stats->etx = packet_etx;
  } else {
//#if LINK_STATS_ETX_WITH_EMANEXT
    /* Compute EMAnext and update ETX */
//#else
    /* Compute EWMA and update ETX */
    stats->etx = ((uint32_t)stats->etx * (EWMA_SCALE - ewma_alpha) +
        (uint32_t)packet_etx * ewma_alpha) / EWMA_SCALE;
//#endif
  }
#endif /* LINK_STATS_ETX_FROM_PACKET_COUNT */
}
/*---------------------------------------------------------------------------*/
/* Packet input callback. Updates statistics for receptions on a given link */
void
link_stats_input_callback(const linkaddr_t *lladdr)
{
  struct link_stats *stats;
  int16_t packet_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  stats = nbr_table_get_from_lladdr(link_stats, lladdr);
  if(stats == NULL) {
    /* Add the neighbor */
    stats = nbr_table_add_lladdr(link_stats, lladdr, NBR_TABLE_REASON_LINK_STATS, NULL);
    if(stats == NULL) {
      return; /* No space left, return */
    }
    initialize_rssi_stats(stats);
  }

  if(stats->rssi[0] == fix16_from_int(LINK_STATS_RSSI_UNKNOWN)) {
    /* Update last Rx timestamp */
    stats->rx_time[0] = clock_time();

    /* Initialize RSSI */
    stats->rssi[0] = fix16_from_int(packet_rssi);
  } else {
#if RPL_DAG_MC == RPL_DAG_MC_SSV
    fix16_t last_rssi;
    clock_time_t last_rx_time = clock_time();
#if LINK_STATS_RSSI_WITH_EMANEXT
    /* Update last RSSI sample using EMAnext. */
    fix16_t diff_s_fix16 = get_seconds_from_ticks(last_rx_time - stats->rx_time[0], CLOCK_SECOND);
    last_rssi = diff_s_fix16 <= fix16_from_int(5*EMA_TAU) ?
                       fix16_ema(stats->rssi[0], fix16_from_int(packet_rssi), diff_s_fix16, fix16_from_int(EMA_TAU)) :
                       fix16_from_int(packet_rssi); /* If weight is very small, do not use it */
#else
    last_rssi = fix16_div(fix16_add(stats->rssi[0] * (EWMA_SCALE - EWMA_ALPHA),
                       fix16_from_int(packet_rssi * EWMA_ALPHA)), fix16_from_int(EWMA_SCALE)); // If alpha == 100: no memory
#endif
    if(last_rx_time - stats->rx_time[0] >= STATIC_DET_TIME_THRESH ||
       fix_abs(fix16_sub(last_rssi, stats->rssi[0])) >= fix16_from_float(STATIC_DET_RSSI_THRESH)) {
      /* Update RSSI and Rx timestamp arrays */
      for(uint8_t i = (LINK_STATS_RSSI_ARR_LEN - 1); i > 0; i--) {
        stats->rx_time[i] = stats->rx_time[i-1];
        stats->rssi[i] = stats->rssi[i-1];
        LOG_DBG("From: ");
        LOG_DBG_LLADDR(lladdr);
        LOG_DBG_(" -> RSSI pos %u: %d, at timestamp pos %u: %lu\n", i, fix16_to_int(stats->rssi[i]), i, stats->rx_time[i]);
      }
      /* Update last Rx timestamp */
      stats->rx_time[0] = last_rx_time;
      /* Update RSSI EMAnext */
      stats->rssi[0] = last_rssi;

      LOG_DBG("From: ");
      LOG_DBG_LLADDR(lladdr);
      LOG_DBG_(" -> RSSI pos 0: %d, at timestamp pos 0: %lu\n", fix16_to_int(stats->rssi[0]), stats->rx_time[0]);
    }
#endif
  }

  stats->link_stats_metric_updated |= 0x0f;

  stats->failed_probes = 0;

  if(stats->etx == 0) {
    /* Initialize ETX */
#if LINK_STATS_INIT_ETX_FROM_RSSI
    stats->etx = guess_etx_from_rssi(stats);
#else /* LINK_STATS_INIT_ETX_FROM_RSSI */
    stats->etx = ETX_DEFAULT * ETX_DIVISOR;
#endif /* LINK_STATS_INIT_ETX_FROM_RSSI */
  }

#if LINK_STATS_PACKET_COUNTERS
  stats->cnt_current.num_packets_rx++;
#endif
}
/*---------------------------------------------------------------------------*/
#if LINK_STATS_PACKET_COUNTERS
/*---------------------------------------------------------------------------*/
static void
print_and_update_counters(void)
{
  struct link_stats *stats;

  for(stats = nbr_table_head(link_stats); stats != NULL;
      stats = nbr_table_next(link_stats, stats)) {

    struct link_packet_counter *c = &stats->cnt_current;

    LOG_INFO("num packets: tx=%u ack=%u rx=%u queue_drops=%u to=",
             c->num_packets_tx, c->num_packets_acked,
             c->num_packets_rx, c->num_queue_drops);
    LOG_INFO_LLADDR(link_stats_get_lladdr(stats));
    LOG_INFO_("\n");

    stats->cnt_total.num_packets_tx += stats->cnt_current.num_packets_tx;
    stats->cnt_total.num_packets_acked += stats->cnt_current.num_packets_acked;
    stats->cnt_total.num_packets_rx += stats->cnt_current.num_packets_rx;
    stats->cnt_total.num_queue_drops += stats->cnt_current.num_queue_drops;
    memset(&stats->cnt_current, 0, sizeof(stats->cnt_current));
  }
}
/*---------------------------------------------------------------------------*/
#endif /* LINK_STATS_PACKET_COUNTERS */
/*---------------------------------------------------------------------------*/
/* Periodic timer called at a period of FRESHNESS_HALF_LIFE */
static void
periodic(void *ptr)
{
  /* Age (by halving) freshness counter of all neighbors */
  struct link_stats *stats;
  ctimer_reset(&periodic_timer);
  for(stats = nbr_table_head(link_stats); stats != NULL; stats = nbr_table_next(link_stats, stats)) {
    stats->freshness >>= 1;
  }

#if LINK_STATS_PACKET_COUNTERS
  print_and_update_counters();
#endif
}
/*---------------------------------------------------------------------------*/
/* Resets link-stats module */
void
link_stats_reset(void)
{
  struct link_stats *stats;
  stats = nbr_table_head(link_stats);
  while(stats != NULL) {
    nbr_table_remove(link_stats, stats);
    stats = nbr_table_next(link_stats, stats);
  }
}
/*---------------------------------------------------------------------------*/
/* Initializes link-stats module */
void
link_stats_init(void)
{
  nbr_table_register(link_stats, NULL);
  ctimer_set(&periodic_timer, FRESHNESS_HALF_LIFE, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
/* Update OF link metric */
void
link_stats_metric_update_callback(const linkaddr_t *lladdr, fix16_t ssv, fix16_t ssr, clock_time_t rx_time)
{
  struct link_stats *stats;
  stats = nbr_table_get_from_lladdr(link_stats, lladdr);
  if(stats != NULL) {
    stats->last_ssv = ssv;
    stats->last_ssr = ssr;
    stats->last_rx_time = rx_time;
    stats->link_stats_metric_updated = 0;
  }
}
/*---------------------------------------------------------------------------*/
/* Update probe time */
void
link_stats_probe_callback(const linkaddr_t *lladdr, clock_time_t probe_time)
{
  struct link_stats *stats;
  stats = nbr_table_get_from_lladdr(link_stats, lladdr);
  if(stats != NULL) {
    stats->last_probe_time = probe_time;
    stats->failed_probes++;
  }
}
/*---------------------------------------------------------------------------*/
/* Update neighbor RSSI */
void
link_stats_nbr_rssi_callback(const linkaddr_t *lladdr, fix16_t par_rssi, clock_time_t time_since)
{
  struct link_stats *stats;
  stats = nbr_table_get_from_lladdr(link_stats, lladdr);
  if(stats == NULL) {
    /* Add the neighbor */
    stats = nbr_table_add_lladdr(link_stats, lladdr, NBR_TABLE_REASON_LINK_STATS, NULL);
    if(stats == NULL) {
      return; /* No space left, return */
    }
    initialize_rssi_stats(stats);
  }

  if(par_rssi != fix16_from_int(LINK_STATS_RSSI_UNKNOWN)) {
    clock_time_t est_rx_time = clock_time() - time_since;
    if(stats->nbr_rssi[0] == par_rssi &&
       est_rx_time < stats->nbr_rx_time[0] + CLOCK_SECOND &&
       est_rx_time > stats->nbr_rx_time[0] - CLOCK_SECOND) {
      LOG_DBG("Duplicate Nbr RSSI ignored\n");
      return;
    }

    for(uint8_t i = (LINK_STATS_RSSI_ARR_LEN - 1); i > 0; i--) {
      stats->nbr_rx_time[i] = stats->nbr_rx_time[i-1];
      stats->nbr_rssi[i] = stats->nbr_rssi[i-1];
      LOG_DBG("From: ");
      LOG_DBG_LLADDR(lladdr);
      LOG_DBG_(" -> Nbr RSSI pos %u: %d, at timestamp pos %u: %lu\n", i, fix16_to_int(stats->nbr_rssi[i]), i, stats->nbr_rx_time[i]);
    }
    /* Update last Rx timestamp */
    stats->nbr_rssi[0] = par_rssi;
    /* Update RSSI EMAnext */
    stats->nbr_rx_time[0] = est_rx_time;

    LOG_DBG("From: ");
    LOG_DBG_LLADDR(lladdr);
    LOG_DBG_(" -> Nbr RSSI pos 0: %d, at timestamp pos 0: %lu\n", fix16_to_int(stats->nbr_rssi[0]), stats->nbr_rx_time[0]);

    stats->link_stats_metric_updated |= 0xf0;
  }
}
