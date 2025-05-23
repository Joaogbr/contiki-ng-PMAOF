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

#if RPL_DAG_MC == RPL_DAG_MC_SSV
#ifdef PMAOF_CONF_DRSSI_SCALE
#define DRSSI_SCALE         PMAOF_CONF_DRSSI_SCALE
#else /* PMAOF_CONF_DRSSI_SCALE */
#define DRSSI_SCALE         (uint16_t)100
#endif /* PMAOF_CONF_DRSSI_SCALE */

#ifdef PMAOF_CONF_MAX_LINK_METRIC
#define MAX_LINK_METRIC         PMAOF_CONF_MAX_LINK_METRIC
#else /* PMAOF_CONF_MAX_LINK_METRIC */
#define MAX_LINK_METRIC         20*DRSSI_SCALE
#endif /* PMAOF_CONF_MAX_LINK_METRIC */

#ifdef PMAOF_CONF_PARENT_SWITCH_THRESHOLD
#define PARENT_SWITCH_THRESHOLD         PMAOF_CONF_PARENT_SWITCH_THRESHOLD
#else /* PMAOF_CONF_PARENT_SWITCH_THRESHOLD */
#define PARENT_SWITCH_THRESHOLD         DRSSI_SCALE
#endif /* PMAOF_CONF_PARENT_SWITCH_THRESHOLD */

#ifdef PMAOF_CONF_MAX_PATH_COST
#define MAX_PATH_COST         PMAOF_CONF_MAX_PATH_COST
#else /* PMAOF_CONF_MAX_PATH_COST */
#define MAX_PATH_COST         320*DRSSI_SCALE
#endif /* PMAOF_CONF_MAX_PATH_COST */

#ifdef PMAOF_CONF_CF_ALPHA
#define CF_ALPHA         PMAOF_CONF_CF_ALPHA
#else /* PMAOF_CONF_CF_ALPHA */
#define CF_ALPHA         6.0f /* Correction factor that multiplies SSV */
#endif /* PMAOF_CONF_CF_ALPHA */

#ifdef PMAOF_CONF_CF_BETA
#define CF_BETA         PMAOF_CONF_CF_BETA
#else /* PMAOF_CONF_CF_BETA */
#define CF_BETA         4.0f /* Determines the point at which deceleration will impact SSV */
#endif /* PMAOF_CONF_CF_BETA */

#ifdef PMAOF_CONF_MAX_ABS_RSSI
#define MAX_ABS_RSSI         PMAOF_CONF_MAX_ABS_RSSI
#else /* PMAOF_CONF_MAX_ABS_RSSI */
#define MAX_ABS_RSSI         100 /* dBm */
#endif /* PMAOF_CONF_MAX_ABS_RSSI */


#ifdef PMAOF_CONF_PATH_COST_RED
#define PATH_COST_RED         PMAOF_CONF_PATH_COST_RED
#else /* PMAOF_CONF_PATH_COST_RED */
#define PATH_COST_RED         20*DRSSI_SCALE /* Max acceptable path cost */
#endif /* PMAOF_CONF_PATH_COST_RED */

#ifdef PMAOF_CONF_SSV_LL_RED
#define SSV_LL_RED         PMAOF_CONF_SSV_LL_RED
#else /* PMAOF_CONF_SSV_LL_RED */
#define SSV_LL_RED         -1*((int16_t)DRSSI_SCALE) /* Min acceptable SSV */
#endif /* PMAOF_CONF_SSV_LL_RED */

#ifdef PMAOF_CONF_SSV_UL_RED
#define SSV_UL_RED         PMAOF_CONF_SSV_UL_RED
#else /* PMAOF_CONF_SSV_UL_RED */
#define SSV_UL_RED         2*DRSSI_SCALE /* Max acceptable SSV */
#endif /* PMAOF_CONF_SSV_UL_RED */

#ifdef PMAOF_CONF_ABS_RSSI_RED
#define ABS_RSSI_RED         PMAOF_CONF_ABS_RSSI_RED
#else /* PMAOF_CONF_ABS_RSSI_RED */
#define ABS_RSSI_RED         87 /* Max acceptable absolute RSSI */
#endif /* PMAOF_CONF_ABS_RSSI_RED */

#ifdef PMAOF_CONF_SSR_RED
#define SSR_RED         PMAOF_CONF_SSR_RED
#else /* PMAOF_CONF_SSR_RED */
#define SSR_RED         ABS_RSSI_RED /* Min acceptable SSR */
#endif /* PMAOF_CONF_SSR_RED */


#ifdef PMAOF_CONF_LINK_COST_LOW_RSSI_COUNT
#define LINK_COST_LOW_RSSI_COUNT         PMAOF_CONF_LINK_COST_LOW_RSSI_COUNT
#else /* PMAOF_CONF_LINK_COST_LOW_RSSI_COUNT */
#define LINK_COST_LOW_RSSI_COUNT         SSV_LL_RED
#endif /* PMAOF_CONF_LINK_COST_LOW_RSSI_COUNT */
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

/* Configuration parameters of RFC6719. Reject parents that have a higher
 * link metric than the following. The default value is 512. */
#define MAX_LINK_METRIC     512 /* Eq ETX of 4 */

/* Reject parents that have a higher path cost than the following. */
#define MAX_PATH_COST      32768   /* Eq path ETX of 256 */

#if !RPL_MRHOF_SQUARED_ETX
/* Hysteresis of MRHOF: the rank must differ more than PARENT_SWITCH_THRESHOLD_DIV
 * in order to switch preferred parent. Default in RFC6719: 192, eq ETX of 1.5. */
#define PARENT_SWITCH_THRESHOLD 192 /* Eq ETX of 1.5 */
#else /* !RPL_MRHOF_SQUARED_ETX */
#define PARENT_SWITCH_THRESHOLD 384 /* Eq ETX of sqrt(3) */
#endif /* !RPL_MRHOF_SQUARED_ETX */
#endif /* RPL_DAG_MC == RPL_DAG_MC_SSV */

/*---------------------------------------------------------------------------*/
#if RPL_WITH_PMAOF
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
static fix16_t get_derivative(fix16_t x0, fix16_t x1, clock_time_t t0, clock_time_t t1)
{
  /* dx/dt */
  fix16_t delta = fix16_sub(x0, x1);
  fix16_t diff_s_fix16 = get_seconds_from_ticks(t0 - t1, CLOCK_SECOND);
  return fix16_div(delta, diff_s_fix16);
}
/*---------------------------------------------------------------------------*/
static fix16_t
get_ssr(fix16_t last_rssi, fix16_t drssi_dt)
{
  /* If the last RSSI value exceeds the threshold, return 0 as the link is too weak. */
  if(fix_abs(last_rssi) > fix16_from_int(ABS_RSSI_RED)) {
    return 0;
  }

  return drssi_dt < 0 ? fix16_sadd(fix16_from_int(ABS_RSSI_RED), last_rssi) :
         fix16_sadd(fix16_from_int(ABS_RSSI_RED), fix_abs(last_rssi));
}
/*---------------------------------------------------------------------------*/
static fix16_t
compute_link_cost(fix16_t ssv, fix16_t ssr)
{
  fix16_t alpha_term = fix16_mul(fix16_from_float(CF_ALPHA), fix_abs(ssv));
  fix16_t beta_term  = fix16_mul(fix16_from_float(CF_BETA), fix16_sub(fix16_from_int(4*ABS_RSSI_RED), ssr));
  return fix16_add(alpha_term, beta_term);
}
#endif
/*---------------------------------------------------------------------------*/
static void
reset(rpl_dag_t *dag)
{
  LOG_INFO("Reset PMAOF\n");
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
  LOG_DBG("PMAOF - DAO ACK received with status: %d\n", status);
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
#if RPL_DAG_MC == RPL_DAG_MC_SSV
    if(stats->link_stats_metric_updated) {
      /* Get the count of available RSSI measurements. */
      uint8_t rssi_cnt = link_stats_get_rssi_count(stats->rssi, stats->rx_time, 0);
      uint8_t nbr_rssi_cnt = link_stats_get_rssi_count(stats->nbr_rssi, stats->nbr_rx_time, 0);
      if(rssi_cnt > 0 || nbr_rssi_cnt > 0) {
        if(rssi_cnt > 1 || nbr_rssi_cnt > 1) {
          /* At least one drssi_dt can be obtained */
          int arr_len = MAX(0, rssi_cnt - 1) + MAX(0, nbr_rssi_cnt - 1);
          fix16_t drssi_dt[arr_len];
          clock_time_t drssi_ts[arr_len];
          arr_len = 0;
          /* If there's at least two samples, of RSSI or Neighbour (Nbr) RSSI, calculate first derivative. */
          if(rssi_cnt > 1) {
            for(int i = 0; i < rssi_cnt - 1; i++) {
              drssi_ts[arr_len] = (((uint64_t)stats->rx_time[i] + (uint64_t)stats->rx_time[i+1]) >> 1);
              if(drssi_ts[arr_len] > stats->last_rx_time) {
                drssi_dt[arr_len] = get_derivative(stats->rssi[i] * DRSSI_SCALE, stats->rssi[i+1] * DRSSI_SCALE,
                                             stats->rx_time[i], stats->rx_time[i+1]);
                arr_len++;
              } else {
                break;
              }
            }
          }
          if(nbr_rssi_cnt > 1) {
            for(int i = 0; i < nbr_rssi_cnt - 1; i++) {
              drssi_ts[arr_len] = (((uint64_t)stats->nbr_rx_time[i] + (uint64_t)stats->nbr_rx_time[i+1]) >> 1);
              if(drssi_ts[arr_len] > stats->last_rx_time) {
                drssi_dt[arr_len] = get_derivative(stats->nbr_rssi[i] * DRSSI_SCALE, stats->nbr_rssi[i+1] * DRSSI_SCALE,
                                             stats->nbr_rx_time[i], stats->nbr_rx_time[i+1]);
                arr_len++;
              } else {
                break;
              }
            }
          }

          if(arr_len) {
            for(int i = 1; i < arr_len; i++) { // Sort in descending order
              clock_time_t temp_ts = drssi_ts[i];
              fix16_t temp_drssi_dt = drssi_dt[i];
              int j = i - 1;
              while(j >= 0 && drssi_ts[j] < temp_ts) {
                drssi_ts[j + 1] = drssi_ts[j];
                drssi_dt[j + 1] = drssi_dt[j];
                j--;
              }
              drssi_ts[j + 1] = temp_ts;
              drssi_dt[j + 1] = temp_drssi_dt;
            }

            fix16_t tau = get_seconds_from_ticks(FRESHNESS_EXPIRATION_TIME, CLOCK_SECOND);
            if(stats->last_rx_time) {
              fix16_t diff_s_fix16 = get_seconds_from_ticks(drssi_ts[arr_len-1] - stats->last_rx_time, CLOCK_SECOND);
              drssi_dt[arr_len-1] = diff_s_fix16 <= 5*tau ?
                                    fix16_ema(stats->last_ssv, drssi_dt[arr_len-1], diff_s_fix16, tau) :
                                    drssi_dt[arr_len-1]; /* If weight is very small, do not use it */
            }
            for(int i = arr_len - 2; i >= 0; i--) {
              fix16_t diff_s_fix16 = get_seconds_from_ticks(drssi_ts[i] - drssi_ts[i+1], CLOCK_SECOND);
              drssi_dt[i] = diff_s_fix16 <= 5*tau ?
                            fix16_ema(drssi_dt[i+1], drssi_dt[i], diff_s_fix16, tau) :
                            drssi_dt[i]; /* If weight is very small, do not use it */
            }
            fix16_t ssv = drssi_dt[0];

            /* Prefer RSSI, but use Nbr RSSI in some circumstances. */
            fix16_t ssr;
            if(!rssi_cnt) {
              ssr = get_ssr(stats->nbr_rssi[0], ssv);
            } else {
              ssr = get_ssr(stats->rssi[0], ssv);
            }

            link_stats_metric_update_callback(rpl_get_parent_lladdr(p), ssv, ssr, drssi_ts[0]);
            /* Link cost is a function of SSV and SSR. */
            return (uint16_t) MIN(fix16_to_int(compute_link_cost(ssv, ssr)), 0xffff);
          }
        }
        fix16_t ssr;
        if(rssi_cnt == 1 && nbr_rssi_cnt == 1) {
          fix16_t ssv;
          if(stats->nbr_rx_time[0] > stats->rx_time[0]) {
            ssv = get_derivative(stats->nbr_rssi[0] * DRSSI_SCALE, stats->rssi[0] * DRSSI_SCALE,
                                stats->nbr_rx_time[0], stats->rx_time[0]);
            ssr = get_ssr(stats->rssi[0], ssv);
          } else {
            ssv = get_derivative(stats->rssi[0] * DRSSI_SCALE, stats->nbr_rssi[0] * DRSSI_SCALE,
                                stats->rx_time[0], stats->nbr_rx_time[0]);
            ssr = get_ssr(stats->rssi[0], ssv);
          }
          link_stats_metric_update_callback(rpl_get_parent_lladdr(p), ssv, ssr, 0);
          return (uint16_t) MIN(fix16_to_int(compute_link_cost(ssv, ssr)), 0xffff);
        }
        /* Punish a bit if only one RSSI reading is available. */
        fix16_t ssv = fix16_from_int(LINK_COST_LOW_RSSI_COUNT);
        if(stats->nbr_rx_time[0] > stats->rx_time[0]) {
          ssr = get_ssr(stats->nbr_rssi[0], fix16_minimum);
        } else {
          ssr = get_ssr(stats->rssi[0], fix16_minimum);
        }
        link_stats_metric_update_callback(rpl_get_parent_lladdr(p), ssv, ssr, 0);
        return (uint16_t) MIN(fix16_to_int(compute_link_cost(ssv, ssr)), 0xffff);
      }
    }
    return (uint16_t) MIN(fix16_to_int(compute_link_cost(stats->last_ssv, stats->last_ssr)), 0xffff);
#else /* RPL_DAG_MC == RPL_DAG_MC_SSV */
#if RPL_MRHOF_SQUARED_ETX
    uint32_t squared_etx = ((uint32_t)stats->etx * stats->etx) / LINK_STATS_ETX_DIVISOR;
    return (uint16_t)MIN(squared_etx, 0xffff);
#else /* RPL_MRHOF_SQUARED_ETX */
    return stats->etx;
#endif /* RPL_MRHOF_SQUARED_ETX */
#endif /* RPL_DAG_MC == RPL_DAG_MC_SSV */
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
  case RPL_DAG_MC_SSV:
    base = p->mc.obj.movfac.ssv;
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
#if RPL_WITH_PMAOF
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

  /* Parent is acceptable if path cost, SSV and SSR do not exceed the thresholds. */
  return p_cost <= PATH_COST_RED * p_hc &&
         stats->last_ssv > fix16_from_int(SSV_LL_RED) &&
         stats->last_ssv <= fix16_from_int(SSV_UL_RED) &&
         stats->last_ssr > fix16_from_int(SSR_RED);
}
#endif
/*---------------------------------------------------------------------------*/
static int
parent_has_usable_link(rpl_parent_t *p)
{
  if(p == NULL || p->dag == NULL || p->dag->instance == NULL) {
    return 0;
  }
#if RPL_WITH_PMAOF
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  /* Exclude links with too high link metrics  */
  return (stats != NULL) && parent_link_metric(p) <= MAX_LINK_METRIC &&
         fix_abs(stats->rssi[0]) <= fix16_from_int(MAX_ABS_RSSI);
#else
  /* Exclude links with too high link metrics  */
  return parent_link_metric(p) <= MAX_LINK_METRIC;
#endif /* RPL_WITH_PMAOF */
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

#if RPL_WITH_PMAOF
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
#endif /* RPL_WITH_PMAOF */

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
  case RPL_DAG_MC_SSV:
    instance->mc.length = 3;
    if(is_root) {
      instance->mc.obj.movfac.hc = 0;
    } else {
      instance->mc.obj.movfac.hc = parent_hop_count(dag->preferred_parent);
    }
    instance->mc.obj.movfac.ssv = path_cost;
    break;
  default:
    LOG_WARN("PMAOF, non-supported MC %u\n", instance->mc.type);
    break;
  }
}
#endif /* RPL_WITH_MC */
/*---------------------------------------------------------------------------*/
rpl_of_t rpl_pmaof = {
  reset,
#if RPL_WITH_DAO_ACK
  dao_ack_callback,
#endif
  parent_link_metric,
  parent_has_usable_link,
  parent_path_cost,
  rank_via_parent,
  best_parent,
  best_dag,
  update_metric_container,
  RPL_OCP_PMAOF,
#if RPL_WITH_PMAOF
  parent_is_acceptable
#endif
};

/** @}*/
