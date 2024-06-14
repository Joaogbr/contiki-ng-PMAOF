/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         The Minimum Rank with Hysteresis Objective Function (MRHOF), RFC6719
 *
 *         This implementation uses the estimated number of
 *         transmissions (ETX) as the additive routing metric,
 *         and also provides stubs for the energy metric.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup uip
 * @{
 */

#include <stdlib.h>
#include "lib/fixmath.h"

#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"

#include "sys/log.h"

#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
#define TX_RANGE            2000.0f /* m */
#define RSSI_RANGE          80.0f /* -10 - (-95) = 85 dBm */
#define DRSSI_SCALE         (uint16_t)100
#define MAX_LINK_METRIC     20*DRSSI_SCALE /* 80 m/s */
#define PARENT_SWITCH_THRESHOLD 5 /* 2 m/s */
#define MAX_PATH_COST       320*DRSSI_SCALE /* 500 m/s */
#define CF_ALPHA            TX_RANGE / RSSI_RANGE
#define CF_BETA             TX_RANGE / (4 * RSSI_RANGE)
#define CF_GAMMA            0.25f
#define RSSI_NOISE_THRESHOLD 0.0f
#define MAX_ABS_RSSI        93 /* dBm */
#define MOVFAC_WITH_ACCEL   1
#define MOVFAC_TAU          30
#define MOVFAC_WITH_EMANEXT 0

#define LINK_COST_RED                   18*DRSSI_SCALE /* 20 m/s */
#define PATH_COST_RED                   (9 * LINK_COST_RED) / 8
#define ABS_RSSI_RED                    89

#define LINK_COST_ORANGE                18*DRSSI_SCALE /* 15 m/s */
#define PATH_COST_ORANGE                (9 * LINK_COST_ORANGE) / 8
#define ABS_RSSI_ORANGE                 85

#define PATH_COST_ORANGE_THRESHOLD      10*DRSSI_SCALE /* 5 m/s */
#define ABS_RSSI_ORANGE_THRESHOLD       20

#define LINK_COST_LOW_RSSI_COUNT              -((int16_t)LINK_COST_RED)

#elif RPL_DAG_MC == RPL_DAG_MC_RSSI
#define MAX_LINK_METRIC     1024 /* dBm */
#define PARENT_SWITCH_THRESHOLD 48 /* dBm */
#define MAX_PATH_COST      4096   /* dBm */
#else
/*
 * RFC6551 and RFC6719 do not mandate the use of a specific formula to
 * compute the ETX value. This MRHOF implementation relies on the
 * value computed by the link-stats module. It has an optional
 * feature, RPL_MRHOF_CONF_SQUARED_ETX, that consists in squaring this
 * value.
 *
 * Squaring basically penalizes bad links while preserving the
 * semantics of ETX (1 = perfect link, more = worse link). As a
 * result, MRHOF will favor good links over short paths. Without this
 * feature, a hop with 50% PRR (ETX=2) is equivalent to two perfect
 * hops with 100% PRR (ETX=1+1=2). With this feature, the former path
 * obtains ETX=2*2=4 and the latter ETX=1*1+1*1=2.
 *
 * While this feature helps achieve extra reliability, it also results
 * in added churn. In networks with high congestion or poor links,
 * this can lead to poor connectivity due to more parent switches,
 * loops, Trickle timer resets, etc.
 */
#ifdef RPL_MRHOF_CONF_SQUARED_ETX
#define RPL_MRHOF_SQUARED_ETX RPL_MRHOF_CONF_SQUARED_ETX
#else /* RPL_MRHOF_CONF_SQUARED_ETX */
#define RPL_MRHOF_SQUARED_ETX 0
#endif /* RPL_MRHOF_CONF_SQUARED_ETX */

#if !RPL_MRHOF_SQUARED_ETX
/* Configuration parameters of RFC6719. Reject parents that have a higher
   link metric than the following. The default value is 512 but we use 1024. */
#define MAX_LINK_METRIC     (8 * ((uint16_t)LINK_STATS_ETX_DIVISOR)) /* Eq ETX of 8 */

/*
 * Hysteresis of MRHOF: the rank must differ more than
 * PARENT_SWITCH_THRESHOLD_DIV in order to switch preferred
 * parent. Default in RFC6719: 192, eq ETX of 1.5.  We use a more
 * aggressive setting: 96, eq ETX of 0.75.
 */
#define PARENT_SWITCH_THRESHOLD (uint8_t)(0.75 * LINK_STATS_ETX_DIVISOR) /* Eq ETX of 0.75 */
#else /* !RPL_MRHOF_SQUARED_ETX */
#define MAX_LINK_METRIC     (16 * ((uint16_t)LINK_STATS_ETX_DIVISOR)) /* Eq ETX of 16 */
#define PARENT_SWITCH_THRESHOLD (uint8_t)(1.25 * LINK_STATS_ETX_DIVISOR) /* Eq ETX of 1.25
                                                                            (results in a
                                                                            churn comparable
                                                                            to the threshold
                                                                            of 96 in the
                                                                            non-squared case) */
#endif /* !RPL_MRHOF_SQUARED_ETX */

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST      (256 * ((uint16_t)LINK_STATS_ETX_DIVISOR))   /* Eq path ETX of 256 */
#endif /* RPL_DAG_MC == RPL_DAG_MC_MOVFAC */

/*---------------------------------------------------------------------------*/
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
static uint16_t
sadd_u16(uint16_t a, uint16_t b)
{
  uint16_t c = a + b;
  return c > a ? c : -1;
}
/*---------------------------------------------------------------------------*/
static uint16_t
ssub_u16(uint16_t a, uint16_t b)
{
  return a > b ? a - b : 0;
}
/*---------------------------------------------------------------------------*/
static fix16_t
get_rrssi(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  if(stats == NULL) {
    return 0;
  }
  /*if((stats->drssi_dt >= 0) != (stats->d2rssi_dt2 >= 0)) {
    fix16_t tp_s = fix16_div(-stats->drssi_dt, stats->d2rssi_dt2);
    fix16_t turn_pt = fix16_add(stats->last_rssi, fix16_add(fix16_mul(stats->drssi_dt, tp_s),
                      fix16_mul(stats->d2rssi_dt2, fix16_sq(tp_s))));
    if(turn_pt >= fix16_from_int(-ABS_RSSI_RED) && turn_pt <= fix16_from_int(ABS_RSSI_RED - 20)) {
      return (stats->drssi_dt >= 0) ?
             fix16_add(fix16_sub(turn_pt, stats->last_rssi),
             fix16_add(fix16_from_int(ABS_RSSI_RED), turn_pt)) :
             fix16_add(fix16_sub(stats->last_rssi, turn_pt),
             fix16_add(fix16_from_int(ABS_RSSI_RED - 20), fix_abs(turn_pt)));
    }
  }
  return (stats->drssi_dt >= 0) ?
         fix16_add(fix16_from_int(ABS_RSSI_RED - 20), fix_abs(stats->last_rssi)) :
         fix16_add(fix16_from_int(ABS_RSSI_RED), stats->last_rssi);*/
  return (stats->last_link_metric >= 0 ?
         fix16_add(fix16_from_int(ABS_RSSI_RED - 20), fix_abs(stats->last_rssi)) :
         fix16_add(fix16_from_int(ABS_RSSI_RED), stats->last_rssi));
}
/*---------------------------------------------------------------------------*/
/*static fix16_t
get_dist_from_rssi(fix16_t rssi)
{
  fix16_t exponent = fix16_div(fix16_sub(RX_SENSITIVITY_DBM, rssi),
                     fix16_mul(fix16_from_int(10), PATH_LOSS_EXPONENT));
  return fix16_mul(fix16_from_float(TX_RANGE), fix16_pow(fix16_from_int(10), exponent));
}*/
#endif
/*---------------------------------------------------------------------------*/
static void
reset(rpl_dag_t *dag)
{
  LOG_INFO("Reset MVMTOF\n");
}
/*---------------------------------------------------------------------------*/
#if RPL_WITH_DAO_ACK
static void
dao_ack_callback(rpl_parent_t *p, int status)
{
  if(status == RPL_DAO_ACK_UNABLE_TO_ADD_ROUTE_AT_ROOT) {
    return;
  }
  /* Here we need to handle failed DAO's and other stuff. */
  LOG_DBG("MVMTOF - DAO ACK received with status: %d\n", status);
  if(status >= RPL_DAO_ACK_UNABLE_TO_ACCEPT) {
    /* punish the ETX as if this was 10 packets lost */
    link_stats_packet_sent(rpl_get_parent_lladdr(p), MAC_TX_OK, 10);
  } else if(status == RPL_DAO_ACK_TIMEOUT) { /* timeout = no ack */
    /* Punish the total lack of ACK with a similar punishment. */
    link_stats_packet_sent(rpl_get_parent_lladdr(p), MAC_TX_OK, 10);
  }
}
#endif /* RPL_WITH_DAO_ACK */
/*---------------------------------------------------------------------------*/
static uint16_t
parent_link_metric(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  if(stats != NULL) {
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
    uint8_t rssi_cnt = link_stats_get_rssi_count(stats, 0);
    if(rssi_cnt > 0) {
      if(stats->link_stats_metric_updated) {
        fix16_t drssi[LINK_STATS_RSSI_ARR_LEN-1] = {0};
        fix16_t drssi_dt[LINK_STATS_RSSI_ARR_LEN-1] = {0};
        fix16_t diff_s_fix16, mf;

        if(rssi_cnt < 2) {
          /* Punish a bit if only one RSSI reading is available */
          mf = fix16_from_int(LINK_COST_LOW_RSSI_COUNT);
        } else {
          for(int i = 0; i < rssi_cnt - 1; i++) {
            drssi[i] = fix16_mul(fix16_sub(stats->rssi[i], stats->rssi[i+1]), fix16_from_int(DRSSI_SCALE));
            diff_s_fix16 = get_seconds_from_ticks(stats->rx_time[i] - stats->rx_time[i+1], CLOCK_SECOND);
            drssi_dt[i] = fix16_div(drssi[i], diff_s_fix16);
          }
          mf = fix16_mul(drssi_dt[0], fix16_from_float(CF_ALPHA));
#if MOVFAC_WITH_ACCEL
          if(rssi_cnt > 2) {
            fix16_t d2rssi_dt[LINK_STATS_RSSI_ARR_LEN-2] = {0};
            fix16_t d2rssi_dt2[LINK_STATS_RSSI_ARR_LEN-2] = {0};
            for(int i = 0; i < rssi_cnt - 2; i++) {
              d2rssi_dt[i] = fix16_sub(drssi_dt[i], drssi_dt[i+1]);
              diff_s_fix16 = get_seconds_from_ticks(stats->rx_time[i] - stats->rx_time[i+1], CLOCK_SECOND);
              d2rssi_dt2[i] = fix16_div(d2rssi_dt[i], diff_s_fix16);
            }

            fix16_t ratio = fix16_div(d2rssi_dt2[0], drssi_dt[0] + !drssi_dt[0]);
            if((d2rssi_dt2[0] >= 0) == (drssi_dt[0] >= 0)) {
              //mf = fix16_add(mf, fix16_mul(d2rssi_dt2[0], fix16_from_float(CF_BETA)));
              //mf = fix16_mul(mf, fix16_add(fix16_one, (ratio < fix16_one ? fix16_sq(ratio) : fix16_sqrt(ratio))));
              //mf = fix16_mul(mf, fix16_add(fix16_one, fix16_div(fix16_sq(ratio), fix16_add(fix16_one, fix16_sq(ratio)))));
              mf = fix16_mul(mf, fix16_add(fix16_one, fix16_log(fix16_add(fix16_one, ratio))));
              //mf = fix16_mul(mf, fix16_exp(ratio));
            } else {
              if(fix_abs(d2rssi_dt2[0]) >= fix_abs(drssi_dt[0])) {
                //mf = -fix16_sub(mf, fix16_mul(d2rssi_dt2[0], fix16_from_float(CF_BETA)));
                mf = fix16_mul(d2rssi_dt2[0], fix16_from_float(CF_ALPHA));
              } else if(fix_abs(ratio) > fix16_from_float(CF_GAMMA)) {
                mf = -mf;
              }
            }
          }
#elif MOVFAC_WITH_EMANEXT
          if(stats->link_stats_metric_updated != 0xff) {
            diff_s_fix16 = get_seconds_from_ticks(stats->rx_time[0] - stats->last_lm_time, CLOCK_SECOND);
            mf = diff_s_fix16 <= fix16_from_int(5*MOVFAC_TAU) ?
                 fix16_ema(stats->last_link_metric, mf, diff_s_fix16, fix16_from_int(MOVFAC_TAU)) :
                 mf;
          }
#endif
          link_stats_metric_update_callback(rpl_get_parent_lladdr(p), mf/*, stats->rx_time[0]*/);
        }

        return (uint16_t) MIN(fix16_to_int(fix_abs(mf)), 0xffff);
      }
      return (uint16_t) MIN(fix16_to_int(fix_abs(stats->last_link_metric)), 0xffff);
    }
#elif RPL_DAG_MC == RPL_DAG_MC_RSSI
    if(stats->last_rssi != fix16_from_int(LINK_STATS_RSSI_UNKNOWN)) {
      int16_t arssi = fix16_to_int(fix16_mul(fix16_from_int(10), fix_abs(stats->last_rssi)));
      LOG_DBG("From: ");
      LOG_DBG_6ADDR(rpl_parent_get_ipaddr(p));
      LOG_DBG_(" -> Current RSSI: %d\n", arssi);
      return (uint16_t)MIN(arssi, 0xffff);
    }
#else /* RPL_DAG_MC == RPL_DAG_MC_MOVFAC */
#if RPL_MRHOF_SQUARED_ETX
    uint32_t squared_etx = ((uint32_t)stats->etx * stats->etx) / LINK_STATS_ETX_DIVISOR;
    return (uint16_t)MIN(squared_etx, 0xffff);
#else /* RPL_MRHOF_SQUARED_ETX */
    return stats->etx;
#endif /* RPL_MRHOF_SQUARED_ETX */
#endif /* RPL_DAG_MC == RPL_DAG_MC_MOVFAC */
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
#if RPL_WITH_MC
static uint8_t
parent_hop_count(rpl_parent_t *p)
{
  uint8_t base;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0x00;
  }

  base = p->mc.obj.movfac.hc;

  return base < 0xff ? base + 1 : base;
}
#endif
/*---------------------------------------------------------------------------*/
static uint16_t
parent_path_cost(rpl_parent_t *p)
{
  uint16_t base;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0xffff;
  }

#if RPL_WITH_MC
  /* Handle the different MC types */
  switch(p->dag->instance->mc.type) {
  case RPL_DAG_MC_ETX:
    base = p->mc.obj.etx;
    break;
  case RPL_DAG_MC_ENERGY:
    base = p->mc.obj.energy.energy_est << 8;
    break;
  case RPL_DAG_MC_RSSI:
    base = p->mc.obj.rssi;
    break;
  case RPL_DAG_MC_MOVFAC:
    base = p->mc.obj.movfac.mf;
    break;
  default:
    base = p->rank;
    break;
  }
#else /* RPL_WITH_MC */
  base = p->rank;
#endif /* RPL_WITH_MC */

  /* path cost upper bound: 0xffff */
  return MIN((uint32_t)base + parent_link_metric(p), 0xffff);
}
/*---------------------------------------------------------------------------*/
static rpl_rank_t
rank_via_parent(rpl_parent_t *p)
{
  uint16_t min_hoprankinc;
  uint16_t path_cost;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return RPL_INFINITE_RANK;
  }

  min_hoprankinc = p->dag->instance->min_hoprankinc;
  path_cost = parent_path_cost(p);

  /* Rank lower-bound: parent rank + min_hoprankinc */
  return MAX(MIN((uint32_t)p->rank + min_hoprankinc, 0xffff), path_cost);
}
/*---------------------------------------------------------------------------*/
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
static int
parent_is_acceptable(rpl_parent_t *p)
{
  uint16_t p_cost = parent_path_cost(p);
  uint8_t p_hc = parent_hop_count(p);
  uint8_t res = 0;
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  if(stats == NULL) {
    return 0;
  } else if(/*stats->last_tx_time > stats->last_rx_time ? stats->etx > 896 :*/
     fix_abs(stats->last_rssi) > fix16_from_int(ABS_RSSI_RED)) {
    res |= 0x01;
  } /*else if(stats->etx > 896) {
    res |= 0x02;
  }*/ else if(fix_abs(stats->last_link_metric) > fix16_from_int(LINK_COST_RED)) {
    res |= 0x04;
  } else if(p_cost > PATH_COST_RED * p_hc) {
    res |= 0x08;
  }
  /*if(p_cost <= PATH_COST_ORANGE * p_hc &&
     fix_abs(stats->last_link_metric) <= fix16_from_int(LINK_COST_ORANGE) &&
     ((fix_abs(stats->last_rssi) <= fix16_from_int(ABS_RSSI_ORANGE) && stats->etx <= 768) ||
     ((stats->last_link_metric >= 0) &&
     (fix_abs(stats->last_rssi) <= fix16_from_int(ABS_RSSI_RED) && stats->etx <= 896)))) {
    res |= 0x20;
  }*/
  return res ? res : 0x10;
}
#endif
/*---------------------------------------------------------------------------*/
static int
parent_has_usable_link(rpl_parent_t *p)
{
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  /* Exclude links with too high link metrics  */
  return (stats != NULL) && parent_link_metric(p) <= MAX_LINK_METRIC &&
         (/*stats->last_tx_time > stats->last_rx_time ? stats->etx <= 1024 :*/
         fix_abs(stats->last_rssi) <= fix16_from_int(MAX_ABS_RSSI));
#else
  /* Exclude links with too high link metrics  */
  return parent_link_metric(p) <= MAX_LINK_METRIC;
#endif /* RPL_DAG_MC == RPL_DAG_MC_MOVFAC */
}
/*---------------------------------------------------------------------------*/
static int
parent_is_usable(rpl_parent_t *p)
{
  /* Exclude links with too high link metrics or path cost. (RFC6719, 3.2.2) */
  return parent_has_usable_link(p) && (parent_path_cost(p) <= MAX_PATH_COST);
}
/*---------------------------------------------------------------------------*/
static rpl_parent_t *
best_parent(rpl_parent_t *p1, rpl_parent_t *p2)
{
  uint16_t p1_cost;
  uint16_t p2_cost;
  int p1_is_usable;
  int p2_is_usable;

  p1_is_usable = p1 != NULL && parent_is_usable(p1);
  p2_is_usable = p2 != NULL && parent_is_usable(p2);

  if(!p1_is_usable) {
    return p2_is_usable ? p2 : NULL;
  }
  if(!p2_is_usable) {
    return p1_is_usable ? p1 : NULL;
  }

#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  int p1_is_acceptable = parent_is_acceptable(p1);
  int p2_is_acceptable = parent_is_acceptable(p2);

  if((p1_is_acceptable & 0xf0) != (p2_is_acceptable & 0xf0)) {
    return (p1_is_acceptable & 0xf0) ? p1 : p2;
  } else if((p1_is_acceptable & 0x03) != (p2_is_acceptable & 0x03)) {
    return (p1_is_acceptable & 0x03) ? p2 : p1;
  }
#endif

  p1_cost = parent_path_cost(p1);
  p2_cost = parent_path_cost(p2);

#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  int16_t diff_rrssi = (int16_t) fix16_to_int(fix16_sub(get_rrssi(p1), get_rrssi(p2)));
  /* Maintain the stability of the preferred parent if performance is acceptable, unless a much better candidate is available. */
  /*rpl_dag_t *dag = p1->dag; // Both parents are in the same DAG.
  if((p1 == dag->preferred_parent || p2 == dag->preferred_parent)) {
    int p1_is_pp = (p1 == dag->preferred_parent);
    int pp_is_acceptable = p1_is_pp ? p1_is_acceptable : p2_is_acceptable;
    if(pp_is_acceptable & 0xf0) {
      int not_pp_is_acceptable = p1_is_pp ? p2_is_acceptable : p1_is_acceptable;
      if(not_pp_is_acceptable > pp_is_acceptable) {
        uint16_t pp_cost = p1_is_pp ? p1_cost : p2_cost;
        uint16_t not_pp_cost = p1_is_pp ? p2_cost : p1_cost;
        uint8_t pp_rrssi = (uint8_t) fix16_to_int(p1_is_pp ? get_rrssi(p1) : get_rrssi(p2));
        uint8_t not_pp_rrssi = (uint8_t) fix16_to_int(p1_is_pp ? get_rrssi(p2) : get_rrssi(p1));
        if(not_pp_cost <= pp_cost && not_pp_rrssi >= pp_rrssi) {
          if(pp_cost - not_pp_cost > PATH_COST_ORANGE_THRESHOLD ||
             not_pp_rrssi - pp_rrssi > ABS_RSSI_ORANGE_THRESHOLD) {
            return p1_is_pp ? p2 : p1;
          }
        }
      }
      return dag->preferred_parent;
    }

    if(p1_cost < p2_cost + DRSSI_SCALE &&
       p1_cost > p2_cost - DRSSI_SCALE &&
       abs(diff_rrssi) < 10) {
      return dag->preferred_parent;
    }
  }*/

  /* If there is no preferred parent, and thus stability is of no concern, go through a series of relations */
  /* Order of priority, highest to lowest: path cost -> link cost -> RSSI -> link cost sign -> new parent? -> hop count -> last rx time */
  uint16_t sw_threshold = ((uint16_t) abs(diff_rrssi)) * PARENT_SWITCH_THRESHOLD;
  if(p1_cost < sadd_u16(p2_cost, sw_threshold) &&
     p1_cost > ssub_u16(p2_cost, sw_threshold)) {
    return diff_rrssi > 0 ? p1 : p2;
  }
#else
  rpl_dag_t *dag = p1->dag; /* Both parents are in the same DAG. */
  /* Maintain the stability of the preferred parent in case of similar ranks. */
  if(p1 == dag->preferred_parent || p2 == dag->preferred_parent) {
    if(p1_cost < p2_cost + PARENT_SWITCH_THRESHOLD &&
       p1_cost > p2_cost - PARENT_SWITCH_THRESHOLD) {
      return dag->preferred_parent;
    }
  }
#endif /* RPL_DAG_MC == RPL_DAG_MC_MOVFAC */

  return p1_cost < p2_cost ? p1 : p2;
}
/*---------------------------------------------------------------------------*/
static rpl_dag_t *
best_dag(rpl_dag_t *d1, rpl_dag_t *d2)
{
  if(d1->grounded != d2->grounded) {
    return d1->grounded ? d1 : d2;
  }

  if(d1->preference != d2->preference) {
    return d1->preference > d2->preference ? d1 : d2;
  }

  return d1->rank < d2->rank ? d1 : d2;
}
/*---------------------------------------------------------------------------*/
#if !RPL_WITH_MC
static void
update_metric_container(rpl_instance_t *instance)
{
  instance->mc.type = RPL_DAG_MC_NONE;
}
#else /* RPL_WITH_MC */
static void
update_metric_container(rpl_instance_t *instance)
{
  rpl_dag_t *dag;
  uint16_t path_cost;
  uint8_t type, is_root;

  dag = instance->current_dag;
  if(dag == NULL || !dag->joined) {
    LOG_WARN("Cannot update the metric container when not joined\n");
    return;
  }

  is_root = (dag->rank == ROOT_RANK(instance));

  if(is_root) {
    /* Configure the metric container at the root only; other nodes
       are auto-configured when joining. */
    instance->mc.type = RPL_DAG_MC;
    instance->mc.flags = 0;
    instance->mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
    instance->mc.prec = 0;
    path_cost = dag->rank;
  } else {
    path_cost = parent_path_cost(dag->preferred_parent);
  }

  /* Handle the different MC types. */
  switch(instance->mc.type) {
  case RPL_DAG_MC_NONE:
    break;
  case RPL_DAG_MC_ETX:
    instance->mc.length = sizeof(instance->mc.obj.etx);
    instance->mc.obj.etx = path_cost;
    break;
  case RPL_DAG_MC_ENERGY:
    instance->mc.length = sizeof(instance->mc.obj.energy);
    if(is_root) {
      type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
    } else {
      type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
    }
    instance->mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
    /* Energy_est is only one byte -- use the least significant byte
       of the path metric. */
    instance->mc.obj.energy.energy_est = path_cost >> 8;
    break;
  case RPL_DAG_MC_RSSI:
    instance->mc.length = sizeof(instance->mc.obj.rssi);
    instance->mc.obj.rssi = path_cost;
    break;
  case RPL_DAG_MC_MOVFAC:
    instance->mc.length = 3;
    if(is_root) {
      instance->mc.obj.movfac.hc = 0;
    } else {
      instance->mc.obj.movfac.hc = parent_hop_count(dag->preferred_parent);
    }
    instance->mc.obj.movfac.mf = path_cost;
    break;
  default:
    LOG_WARN("MVMTOF, non-supported MC %u\n", instance->mc.type);
    break;
  }
}
#endif /* RPL_WITH_MC */
/*---------------------------------------------------------------------------*/
rpl_of_t rpl_mvmtof = {
  reset,
#if RPL_WITH_DAO_ACK
  dao_ack_callback,
#endif
  parent_link_metric,
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  parent_is_acceptable,
#endif
  parent_has_usable_link,
  parent_path_cost,
  rank_via_parent,
  best_parent,
  best_dag,
  update_metric_container,
  RPL_OCP_MVMTOF
};

/** @}*/
