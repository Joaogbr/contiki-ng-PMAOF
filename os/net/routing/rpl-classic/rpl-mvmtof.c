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

#if 1
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
#ifdef MVMTOF_CONF_DRSSI_SCALE
#define DRSSI_SCALE         MVMTOF_CONF_DRSSI_SCALE
#else /* MVMTOF_CONF_DRSSI_SCALE */
#define DRSSI_SCALE         (uint16_t)100
#endif /* MVMTOF_CONF_DRSSI_SCALE */

#ifdef MVMTOF_CONF_MAX_LINK_METRIC
#define MAX_LINK_METRIC         MVMTOF_CONF_MAX_LINK_METRIC
#else /* MVMTOF_CONF_MAX_LINK_METRIC */
#define MAX_LINK_METRIC         20*DRSSI_SCALE
#endif /* MVMTOF_CONF_MAX_LINK_METRIC */

#ifdef MVMTOF_CONF_PARENT_SWITCH_THRESHOLD
#define PARENT_SWITCH_THRESHOLD         MVMTOF_CONF_PARENT_SWITCH_THRESHOLD
#else /* MVMTOF_CONF_PARENT_SWITCH_THRESHOLD */
#define PARENT_SWITCH_THRESHOLD         DRSSI_SCALE
#endif /* MVMTOF_CONF_PARENT_SWITCH_THRESHOLD */

#ifdef MVMTOF_CONF_MAX_PATH_COST
#define MAX_PATH_COST         MVMTOF_CONF_MAX_PATH_COST
#else /* MVMTOF_CONF_MAX_PATH_COST */
#define MAX_PATH_COST         320*DRSSI_SCALE
#endif /* MVMTOF_CONF_MAX_PATH_COST */

#ifdef MVMTOF_CONF_CF_ALPHA
#define CF_ALPHA         MVMTOF_CONF_CF_ALPHA
#else /* MVMTOF_CONF_CF_ALPHA */
#define CF_ALPHA         25.0f /* Correction factor that multiplies MF */
#endif /* MVMTOF_CONF_CF_ALPHA */

#ifdef MVMTOF_CONF_CF_BETA
#define CF_BETA         MVMTOF_CONF_CF_BETA
#else /* MVMTOF_CONF_CF_BETA */
#define CF_BETA         0.25f /* Determines the point at which deceleration will impact MF */
#endif /* MVMTOF_CONF_CF_BETA */

#ifdef MVMTOF_CONF_MAX_ABS_RSSI
#define MAX_ABS_RSSI         MVMTOF_CONF_MAX_ABS_RSSI
#else /* MVMTOF_CONF_MAX_ABS_RSSI */
#define MAX_ABS_RSSI         94 /* dBm */
#endif /* MVMTOF_CONF_MAX_ABS_RSSI */


#ifdef MVMTOF_CONF_PATH_COST_RED
#define PATH_COST_RED         MVMTOF_CONF_PATH_COST_RED
#else /* MVMTOF_CONF_PATH_COST_RED */
#define PATH_COST_RED         20*DRSSI_SCALE /* Max acceptable path cost */
#endif /* MVMTOF_CONF_PATH_COST_RED */

#ifdef MVMTOF_CONF_MF_LL_RED
#define MF_LL_RED         MVMTOF_CONF_MF_LL_RED
#else /* MVMTOF_CONF_MF_LL_RED */
#define MF_LL_RED         -4*((int16_t)DRSSI_SCALE) /* Min acceptable MF */
#endif /* MVMTOF_CONF_MF_LL_RED */

#ifdef MVMTOF_CONF_MF_UL_RED
#define MF_UL_RED         MVMTOF_CONF_MF_UL_RED
#else /* MVMTOF_CONF_MF_UL_RED */
#define MF_UL_RED         8*DRSSI_SCALE /* Max acceptable MF */
#endif /* MVMTOF_CONF_MF_UL_RED */

#ifdef MVMTOF_CONF_ABS_RSSI_RED
#define ABS_RSSI_RED         MVMTOF_CONF_ABS_RSSI_RED
#else /* MVMTOF_CONF_ABS_RSSI_RED */
#define ABS_RSSI_RED         93 /* Max acceptable absolute RSSI */
#endif /* MVMTOF_CONF_ABS_RSSI_RED */

#ifdef MVMTOF_CONF_RRSSI_RED
#define RRSSI_RED         MVMTOF_CONF_RRSSI_RED
#else /* MVMTOF_CONF_RRSSI_RED */
#define RRSSI_RED         60 /* Min acceptable RRSSI */
#endif /* MVMTOF_CONF_RRSSI_RED */


#ifdef MVMTOF_CONF_LINK_COST_LOW_RSSI_COUNT
#define LINK_COST_LOW_RSSI_COUNT         MVMTOF_CONF_LINK_COST_LOW_RSSI_COUNT
#else /* MVMTOF_CONF_LINK_COST_LOW_RSSI_COUNT */
#define LINK_COST_LOW_RSSI_COUNT         MF_LL_RED
#endif /* MVMTOF_CONF_LINK_COST_LOW_RSSI_COUNT */

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
get_rrssi(fix16_t last_rssi, fix16_t drssi_dt, fix16_t d2rssi_dt2)
{
  /* If the last RSSI value exceeds the threshold, return 0 as the link is too weak. */
  if(fix_abs(last_rssi) > fix16_from_int(ABS_RSSI_RED)) {
    return 0;
  }

  /* Check if there is deceleration. */
  if(d2rssi_dt2 != 0 && ((drssi_dt >= 0) != (d2rssi_dt2 >= 0))) {
    fix16_t tp_s = fix_abs(fix16_sdiv(drssi_dt, d2rssi_dt2)); // Time to reach the turning point.
    fix16_t turn_pt = fix16_sadd(last_rssi, fix16_sadd(fix16_smul(drssi_dt, tp_s),
                      fix16_smul(fix16_div(d2rssi_dt2, fix16_from_int(2)),
                      fix16_smul(tp_s, tp_s)))); // RSSI value of the turning point.

    /* If the turning point is within a valid range, use it calculate remaining RSSI. */
    if(drssi_dt >= 0 && turn_pt <= fix16_from_int(ABS_RSSI_RED - 20)) {
      return fix16_sadd(fix16_ssub(turn_pt, last_rssi),
             fix16_sadd(fix16_from_int(ABS_RSSI_RED), turn_pt));
    } else if(drssi_dt < 0 && turn_pt >= fix16_from_int(-ABS_RSSI_RED)) {
      return fix16_sadd(fix16_ssub(last_rssi, turn_pt),
             fix16_sadd(fix16_from_int(ABS_RSSI_RED - 20), fix_abs(turn_pt)));
    }
  }
  return drssi_dt < 0 ? fix16_sadd(fix16_from_int(ABS_RSSI_RED), last_rssi) :
         fix16_sadd(fix16_from_int(ABS_RSSI_RED - 20), fix_abs(last_rssi));
}
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
    /* Get the count of available RSSI measurements. */
    uint8_t rssi_cnt = link_stats_get_rssi_count(stats, 0);
    if(rssi_cnt > 0) {
      if(rssi_cnt > 1) {
        if(stats->link_stats_metric_updated) {
          fix16_t drssi[LINK_STATS_RSSI_ARR_LEN-1] = {0};
          fix16_t drssi_dt[LINK_STATS_RSSI_ARR_LEN-1] = {0};
          fix16_t diff_s_fix16, mf;
          /* If there's at least two samples, calculate first derivative of RSSI. */
          for(int i = 0; i < rssi_cnt - 1; i++) {
            drssi[i] = fix16_mul(fix16_sub(stats->rssi[i], stats->rssi[i+1]), fix16_from_int(DRSSI_SCALE));
            diff_s_fix16 = get_seconds_from_ticks(stats->rx_time[i] - stats->rx_time[i+1], CLOCK_SECOND);
            drssi_dt[i] = fix16_div(drssi[i], diff_s_fix16);
          }
          /* Calculate MF solely based on drssi_dt for now. */
          mf = fix16_mul(drssi_dt[0], fix16_from_float(CF_ALPHA));
          fix16_t d2rssi_dt[LINK_STATS_RSSI_ARR_LEN-2] = {0};
          fix16_t d2rssi_dt2[LINK_STATS_RSSI_ARR_LEN-2] = {0};
          if(rssi_cnt > 2) {
            /* If there's at least three samples, calculate second derivative of RSSI. */
            for(int i = 0; i < rssi_cnt - 2; i++) {
              d2rssi_dt[i] = fix16_sub(drssi_dt[i], drssi_dt[i+1]);
              diff_s_fix16 = get_seconds_from_ticks(stats->rx_time[i] - stats->rx_time[i+1], CLOCK_SECOND);
              d2rssi_dt2[i] = fix16_div(d2rssi_dt[i], diff_s_fix16);
            }
            /* Update MF based on d2rssi_dt2. */
            if((d2rssi_dt2[0] >= 0) == (drssi_dt[0] >= 0)) {
              fix16_t ratio = fix16_div(d2rssi_dt2[0], drssi_dt[0] + !drssi_dt[0]);
              mf = fix16_mul(mf, fix16_add(fix16_one, fix16_log(fix16_add(fix16_one, ratio))));
            } else {
              if(fix_abs(d2rssi_dt2[0]) >= fix_abs(drssi_dt[0])) {
                mf = fix16_mul(d2rssi_dt2[0], fix16_from_float(CF_ALPHA));
              } else if(fix_abs(d2rssi_dt2[0]) > fix16_mul(fix_abs(drssi_dt[0]), fix16_from_float(CF_BETA))) {
                mf = -mf;
              }
            }
          }

          /* Link cost is a function of MF and RRSSI. */
          fix16_t rrssi = get_rrssi(stats->rssi[0], drssi_dt[0], d2rssi_dt2[0]);
          link_stats_metric_update_callback(rpl_get_parent_lladdr(p), mf, rrssi);
          return (uint16_t) MIN(fix16_to_int(fix16_add(fix_abs(mf),
                 fix16_sub(fix16_from_int(4*ABS_RSSI_RED), rrssi))), 0xffff);
        }
        return (uint16_t) MIN(fix16_to_int(fix16_add(fix_abs(stats->last_mf),
               fix16_sub(fix16_from_int(4*ABS_RSSI_RED), stats->last_rrssi))), 0xffff);
      }
      /* Punish a bit if only one RSSI reading is available. */
      return (uint16_t) abs(LINK_COST_LOW_RSSI_COUNT);
    }
#elif RPL_DAG_MC == RPL_DAG_MC_RSSI
    if(stats->rssi[0] != fix16_from_int(LINK_STATS_RSSI_UNKNOWN)) {
      int16_t arssi = fix16_to_int(fix16_mul(fix16_from_int(10), fix_abs(stats->rssi[0])));
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
static uint8_t
parent_is_acceptable(rpl_parent_t *p)
{
  uint16_t p_cost;
  uint8_t p_hc;
  const struct link_stats *stats;

  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0;
  }

  p_cost = parent_path_cost(p);
  p_hc = parent_hop_count(p);
  stats = rpl_get_parent_link_stats(p);

  if(stats == NULL) {
    return 0;
  }

  /* Parent is acceptable if path cost, MF and RRSSI do not exceed the thresholds. */
  return p_cost <= PATH_COST_RED * p_hc &&
         stats->last_mf > fix16_from_int(MF_LL_RED) &&
         stats->last_mf <= fix16_from_int(MF_UL_RED) &&
         stats->last_rrssi > fix16_from_int(RRSSI_RED);
}
#endif
/*---------------------------------------------------------------------------*/
static int
parent_has_usable_link(rpl_parent_t *p)
{
  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0;
  }
#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  /* Exclude links with too high link metrics  */
  return (stats != NULL) && parent_link_metric(p) <= MAX_LINK_METRIC &&
         fix_abs(stats->rssi[0]) <= fix16_from_int(MAX_ABS_RSSI);
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

  p1_cost = parent_path_cost(p1);
  p2_cost = parent_path_cost(p2);

#if RPL_DAG_MC == RPL_DAG_MC_MOVFAC
  rpl_dag_t *dag = p1->dag; // Both parents are in the same DAG.
  if((p1 == dag->preferred_parent || p2 == dag->preferred_parent)) {
    if(p1_cost < sadd_u16(p2_cost, PARENT_SWITCH_THRESHOLD) &&
       p1_cost > ssub_u16(p2_cost, PARENT_SWITCH_THRESHOLD)) {
      return dag->preferred_parent;
    }
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
#endif

/** @}*/
