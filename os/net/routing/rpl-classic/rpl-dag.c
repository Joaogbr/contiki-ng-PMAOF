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
 *         Logic for Directed Acyclic Graphs in RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 * Contributors: George Oikonomou <oikonomou@users.sourceforge.net> (multicast)
 */

/**
 * \addtogroup uip
 * @{
 */

#include "contiki.h"
#include "net/link-stats.h"
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#include "net/routing/rpl-classic/rpl-dag-root.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/nbr-table.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/ctimer.h"
#include "sys/log.h"

#include <limits.h>
#include <string.h>

#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/* A configurable function called after every RPL parent switch. */
#ifdef RPL_CALLBACK_PARENT_SWITCH
void RPL_CALLBACK_PARENT_SWITCH(rpl_parent_t *old, rpl_parent_t *new);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

/*---------------------------------------------------------------------------*/
extern rpl_of_t rpl_of0, rpl_mrhof, rpl_pmaof;
static rpl_of_t *const objective_functions[] = RPL_SUPPORTED_OFS;

/*---------------------------------------------------------------------------*/
/* RPL definitions. */

#ifndef RPL_CONF_GROUNDED
#define RPL_GROUNDED                    0
#else
#define RPL_GROUNDED                    RPL_CONF_GROUNDED
#endif /* !RPL_CONF_GROUNDED */

/*---------------------------------------------------------------------------*/
/* Per-parent RPL information */
NBR_TABLE_GLOBAL(rpl_parent_t, rpl_parents);
/*---------------------------------------------------------------------------*/
/* Allocate instance table. */
rpl_instance_t instance_table[RPL_MAX_INSTANCES];
rpl_instance_t *default_instance;

/*---------------------------------------------------------------------------*/
void
rpl_print_neighbor_list(void)
{
  if(default_instance != NULL && default_instance->current_dag != NULL &&
     default_instance->of != NULL) {
    int curr_dio_interval = default_instance->dio_intcurrent;
    int curr_rank = default_instance->current_dag->rank;
    rpl_parent_t *p = nbr_table_head(rpl_parents);
    clock_time_t clock_now = clock_time();

    LOG_DBG("RPL: MOP %u OCP %u rank %u dioint %u, nbr count %u\n",
            default_instance->mop, default_instance->of->ocp, curr_rank, curr_dio_interval, uip_ds6_nbr_num());
    while(p != NULL) {
      const struct link_stats *stats = rpl_get_parent_link_stats(p);
      uip_ipaddr_t *parent_addr = rpl_parent_get_ipaddr(p);
      LOG_DBG("RPL: nbr %02x %5u, %5u => %5u -- %2u %c%c (last tx %u min ago)\n",
              parent_addr != NULL ? parent_addr->u8[15] : 0x0,
              p->rank,
              rpl_get_parent_link_metric(p),
              rpl_rank_via_parent(p),
              stats != NULL ? stats->freshness : 0,
              link_stats_tx_fresh(stats, FRESHNESS_EXPIRATION_TIME) ? 'f' : ' ',
              p == default_instance->current_dag->preferred_parent ? 'p' : ' ',
              stats != NULL ? (unsigned)((clock_now - stats->last_tx_time) / (60 * CLOCK_SECOND)) : -1u
              );
      p = nbr_table_next(rpl_parents, p);
    }
    LOG_DBG("RPL: end of list\n");
  }
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
rpl_get_nbr(rpl_parent_t *parent)
{
  const linkaddr_t *lladdr = rpl_get_parent_lladdr(parent);
  if(lladdr != NULL) {
    return uip_ds6_nbr_ll_lookup((const uip_lladdr_t *)lladdr);
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
nbr_callback(void *ptr)
{
  rpl_remove_parent(ptr);
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_init(void)
{
  nbr_table_register(rpl_parents, (nbr_table_callback *)nbr_callback);
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_get_parent(const uip_lladdr_t *addr)
{
  return nbr_table_get_from_lladdr(rpl_parents, (const linkaddr_t *)addr);
}
/*---------------------------------------------------------------------------*/
rpl_rank_t
rpl_get_parent_rank(uip_lladdr_t *addr)
{
  rpl_parent_t *p = nbr_table_get_from_lladdr(rpl_parents, (linkaddr_t *)addr);
  if(p != NULL) {
    return p->rank;
  }

  return RPL_INFINITE_RANK;
}
/*---------------------------------------------------------------------------*/
uint16_t
rpl_get_parent_link_metric(rpl_parent_t *p)
{
  if(p != NULL && p->dag != NULL) {
    rpl_instance_t *instance = p->dag->instance;
    if(instance != NULL && instance->of != NULL &&
       instance->of->parent_link_metric != NULL) {
      return instance->of->parent_link_metric(p);
    }
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
rpl_rank_t
rpl_rank_via_parent(rpl_parent_t *p)
{
  if(p != NULL && p->dag != NULL) {
    rpl_instance_t *instance = p->dag->instance;
    if(instance != NULL && instance->of != NULL &&
       instance->of->rank_via_parent != NULL) {
      return instance->of->rank_via_parent(p);
    }
  }
  return RPL_INFINITE_RANK;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
rpl_get_parent_lladdr(rpl_parent_t *p)
{
  return nbr_table_get_lladdr(rpl_parents, p);
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
rpl_parent_get_ipaddr(rpl_parent_t *p)
{
  const linkaddr_t *lladdr = rpl_get_parent_lladdr(p);
  if(lladdr == NULL) {
    return NULL;
  }
  return uip_ds6_nbr_ipaddr_from_lladdr((uip_lladdr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
const struct link_stats *
rpl_get_parent_link_stats(rpl_parent_t *p)
{
  const linkaddr_t *lladdr = rpl_get_parent_lladdr(p);
  return link_stats_from_lladdr(lladdr);
}
/*---------------------------------------------------------------------------*/
#if RPL_WITH_PMAOF
uint16_t
rpl_get_parent_path_cost(rpl_parent_t *p)
{
  if(p != NULL && p->dag != NULL) {
    rpl_instance_t *instance = p->dag->instance;
    if(instance != NULL && instance->of != NULL &&
       instance->of->parent_path_cost != NULL) {
      return instance->of->parent_path_cost(p);
    }
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
int
rpl_parent_probe_recent(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  if(stats == NULL) {
    return 0;
  }
  if(stats->failed_probes > LINK_STATS_FAILED_PROBES_MAX_NUM) {
    return link_stats_recent_probe(stats, FRESHNESS_EXPIRATION_TIME);
  }
  return link_stats_recent_probe(stats, FRESHNESS_EXPIRATION_TIME >> 1);
}
/*---------------------------------------------------------------------------*/
int
rpl_pref_parent_rx_fresh(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
  return link_stats_rx_fresh(stats, FRESHNESS_EXPIRATION_TIME >> 1) ||
         link_stats_recent_probe(stats, FRESHNESS_EXPIRATION_TIME >> 2);
}
/*---------------------------------------------------------------------------*/
#endif
int
rpl_parent_is_fresh(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_get_parent_link_stats(p);
#if RPL_DAG_MC == RPL_DAG_MC_SSV
  return link_stats_rx_fresh(stats, FRESHNESS_EXPIRATION_TIME);
#else
  return link_stats_tx_fresh(stats, FRESHNESS_EXPIRATION_TIME);
#endif
}
/*---------------------------------------------------------------------------*/
int
rpl_parent_is_reachable(rpl_parent_t *p)
{
  if(p == NULL || p->dag == NULL || p->dag->instance == NULL ||
     p->dag->instance->of == NULL) {
    return 0;
  }

#if UIP_ND6_SEND_NS
  /* Exclude links to a neighbor that is not reachable at a NUD level */
  if(rpl_get_nbr(p) == NULL) {
    return 0;
  }
#endif /* UIP_ND6_SEND_NS */

  /* If we don't have fresh link information, assume the parent is reachable. */
  return !rpl_parent_is_fresh(p) ||
         p->dag->instance->of->parent_has_usable_link(p);
}
/*---------------------------------------------------------------------------*/
static void
rpl_set_preferred_parent(rpl_dag_t *dag, rpl_parent_t *p)
{
  if(dag == NULL || dag->preferred_parent == p) {
    return;
  }

  LOG_INFO("rpl_set_preferred_parent: used to be ");
  if(dag->preferred_parent != NULL) {
    LOG_INFO_6ADDR(rpl_parent_get_ipaddr(dag->preferred_parent));
  } else {
    LOG_INFO_("NULL");
  }
  LOG_INFO_(", now is ");
  if(p != NULL) {
    LOG_INFO_6ADDR(rpl_parent_get_ipaddr(p));
    LOG_INFO_("\n");
    LOG_INFO("new parent lladdr -> ");
    LOG_INFO_LLADDR(rpl_get_parent_lladdr(p));
    LOG_INFO_("\n");
  } else {
    LOG_INFO_("NULL\n");
    if(!rpl_has_joined()) {
      LOG_INFO("node has left the network\n");
    }
  }

#ifdef RPL_CALLBACK_PARENT_SWITCH
  RPL_CALLBACK_PARENT_SWITCH(dag->preferred_parent, p);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

  /* Always keep the preferred parent locked, so it remains in the
   * neighbor table. */
  nbr_table_unlock(rpl_parents, dag->preferred_parent);
  nbr_table_lock(rpl_parents, p);
  dag->preferred_parent = p;
}
/*---------------------------------------------------------------------------*/
/* Greater-than function for the lollipop counter.                      */
/*---------------------------------------------------------------------------*/
static int
lollipop_greater_than(int a, int b)
{
  /* Check if we are comparing an initial value with an old value. */
  if(a > RPL_LOLLIPOP_CIRCULAR_REGION && b <= RPL_LOLLIPOP_CIRCULAR_REGION) {
    return (RPL_LOLLIPOP_MAX_VALUE + 1 + b - a) > RPL_LOLLIPOP_SEQUENCE_WINDOWS;
  }
  /* Otherwise check if a > b and comparable => ok, or
     if they have wrapped and are still comparable. */
  return (a > b && (a - b) < RPL_LOLLIPOP_SEQUENCE_WINDOWS) ||
         (a < b && (b - a) > (RPL_LOLLIPOP_CIRCULAR_REGION + 1 - RPL_LOLLIPOP_SEQUENCE_WINDOWS));
}
/*---------------------------------------------------------------------------*/
/* Remove DAG parents with a rank that is at least the same as minimum_rank. */
static void
remove_parents(rpl_dag_t *dag, rpl_rank_t minimum_rank)
{
  rpl_parent_t *p;

  LOG_INFO("Removing parents (minimum rank %u)\n", minimum_rank);

  p = nbr_table_head(rpl_parents);
  while(p != NULL) {
    if(dag == p->dag && p->rank >= minimum_rank) {
      rpl_remove_parent(p);
    }
    p = nbr_table_next(rpl_parents, p);
  }
}
/*---------------------------------------------------------------------------*/
static void
nullify_parents(rpl_dag_t *dag, rpl_rank_t minimum_rank)
{
  rpl_parent_t *p;

  LOG_INFO("Nullifying parents (minimum rank %u)\n", minimum_rank);

  p = nbr_table_head(rpl_parents);
  while(p != NULL) {
    if(dag == p->dag && p->rank >= minimum_rank) {
      rpl_nullify_parent(p);
    }
    p = nbr_table_next(rpl_parents, p);
  }
}
/*---------------------------------------------------------------------------*/
static int
should_refresh_routes(rpl_instance_t *instance, rpl_dio_t *dio, rpl_parent_t *p)
{
  /* If MOP is set to no downward routes, then no DAO should be sent. */
  if(instance->mop == RPL_MOP_NO_DOWNWARD_ROUTES) {
    return 0;
  }

  /* Check if the new DTSN is more recent. */
  return p == instance->current_dag->preferred_parent &&
         (lollipop_greater_than(dio->dtsn, p->dtsn));
}
/*---------------------------------------------------------------------------*/
static int
acceptable_rank(rpl_dag_t *dag, rpl_rank_t rank)
{
  return rank != RPL_INFINITE_RANK &&
         ((dag->instance->max_rankinc == 0) ||
          DAG_RANK(rank, dag->instance) <= DAG_RANK(dag->min_rank + dag->instance->max_rankinc, dag->instance));
}
/*---------------------------------------------------------------------------*/
static rpl_dag_t *
get_dag(uint8_t instance_id, uip_ipaddr_t *dag_id)
{
  rpl_instance_t *instance;
  rpl_dag_t *dag;
  int i;

  instance = rpl_get_instance(instance_id);
  if(instance == NULL) {
    return NULL;
  }

  for(i = 0; i < RPL_MAX_DAG_PER_INSTANCE; ++i) {
    dag = &instance->dag_table[i];
    if(dag->used && uip_ipaddr_cmp(&dag->dag_id, dag_id)) {
      return dag;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_set_root(uint8_t instance_id, uip_ipaddr_t *dag_id)
{
  rpl_dag_t *dag;
  rpl_instance_t *instance;
  uint8_t version;
  int i;

  version = RPL_LOLLIPOP_INIT;
  instance = rpl_get_instance(instance_id);
  if(instance != NULL) {
    for(i = 0; i < RPL_MAX_DAG_PER_INSTANCE; ++i) {
      dag = &instance->dag_table[i];
      if(dag->used) {
        if(uip_ipaddr_cmp(&dag->dag_id, dag_id)) {
          version = dag->version;
          RPL_LOLLIPOP_INCREMENT(version);
        } else {
          if(dag == dag->instance->current_dag) {
            LOG_INFO("Dropping a joined DAG when setting this node as root\n");
            rpl_set_default_route(instance, NULL);
            dag->instance->current_dag = NULL;
          } else {
            LOG_INFO("Dropping a DAG when setting this node as root\n");
          }
          rpl_free_dag(dag);
        }
      }
    }
  }

  dag = rpl_alloc_dag(instance_id, dag_id);
  if(dag == NULL) {
    LOG_ERR("Failed to allocate a DAG\n");
    return NULL;
  }

  instance = dag->instance;

  dag->version = version;
  dag->joined = 1;
  dag->grounded = RPL_GROUNDED;
  dag->preference = RPL_PREFERENCE;
  instance->mop = RPL_MOP_DEFAULT;
  instance->of = rpl_find_of(RPL_OF_OCP);
  if(instance->of == NULL) {
    LOG_WARN("OF with OCP %u not supported\n", RPL_OF_OCP);
    return NULL;
  }

  rpl_set_preferred_parent(dag, NULL);

  memcpy(&dag->dag_id, dag_id, sizeof(dag->dag_id));

  instance->dio_intdoubl = RPL_DIO_INTERVAL_DOUBLINGS;
  instance->dio_intmin = RPL_DIO_INTERVAL_MIN;
  /* The current interval must differ from the minimum interval in order to
     trigger a DIO timer reset. */
  instance->dio_intcurrent = RPL_DIO_INTERVAL_MIN +
    RPL_DIO_INTERVAL_DOUBLINGS;
  instance->dio_redundancy = RPL_DIO_REDUNDANCY;
  instance->max_rankinc = RPL_MAX_RANKINC;
  instance->min_hoprankinc = RPL_MIN_HOPRANKINC;
  instance->default_lifetime = RPL_DEFAULT_LIFETIME;
  instance->lifetime_unit = RPL_DEFAULT_LIFETIME_UNIT;

  dag->rank = ROOT_RANK(instance);

  if(instance->current_dag != dag && instance->current_dag != NULL) {
    /* Remove routes installed by DAOs. */
    if(RPL_IS_STORING(instance)) {
      rpl_remove_routes(instance->current_dag);
    }

    instance->current_dag->joined = 0;
  }

  instance->current_dag = dag;
  instance->dtsn_out = RPL_LOLLIPOP_INIT;
  instance->of->update_metric_container(instance);
  default_instance = instance;

  LOG_INFO("Node set to be a DAG root with DAG ID ");
  LOG_INFO_6ADDR(&dag->dag_id);
  LOG_INFO_("\n");

  LOG_ANNOTATE("#A root=%u\n", dag->dag_id.u8[sizeof(dag->dag_id) - 1]);

  rpl_reset_dio_timer(instance);

  return dag;
}
/*---------------------------------------------------------------------------*/
int
rpl_repair_root(uint8_t instance_id)
{
  rpl_instance_t *instance;

  instance = rpl_get_instance(instance_id);
  if(instance == NULL ||
     instance->current_dag->rank != ROOT_RANK(instance)) {
    LOG_WARN("rpl_repair_root triggered but not root\n");
    return 0;
  }
  RPL_STAT(rpl_stats.root_repairs++);

  RPL_LOLLIPOP_INCREMENT(instance->current_dag->version);
  RPL_LOLLIPOP_INCREMENT(instance->dtsn_out);
  LOG_INFO("rpl_repair_root initiating global repair with version %d\n",
           instance->current_dag->version);
  rpl_reset_dio_timer(instance);
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
set_ip_from_prefix(uip_ipaddr_t *ipaddr, rpl_prefix_t *prefix)
{
  memset(ipaddr, 0, sizeof(uip_ipaddr_t));
  memcpy(ipaddr, &prefix->prefix, (prefix->length + 7) / 8);
  uip_ds6_set_addr_iid(ipaddr, &uip_lladdr);
}
/*---------------------------------------------------------------------------*/
static void
check_prefix(rpl_prefix_t *last_prefix, rpl_prefix_t *new_prefix)
{
  uip_ipaddr_t ipaddr;
  uip_ds6_addr_t *rep;

  if(last_prefix != NULL && new_prefix != NULL &&
     last_prefix->length == new_prefix->length &&
     uip_ipaddr_prefixcmp(&last_prefix->prefix,
                          &new_prefix->prefix, new_prefix->length) &&
     last_prefix->flags == new_prefix->flags) {
    /* Nothing has changed. */
    return;
  }

  if(last_prefix != NULL) {
    set_ip_from_prefix(&ipaddr, last_prefix);
    rep = uip_ds6_addr_lookup(&ipaddr);
    if(rep != NULL) {
      LOG_DBG("removing global IP address ");
      LOG_DBG_6ADDR(&ipaddr);
      LOG_DBG_("\n");
      uip_ds6_addr_rm(rep);
    }
  }

  if(new_prefix != NULL) {
    set_ip_from_prefix(&ipaddr, new_prefix);
    if(uip_ds6_addr_lookup(&ipaddr) == NULL) {
      LOG_DBG("adding global IP address ");
      LOG_DBG_6ADDR(&ipaddr);
      LOG_DBG_("\n");
      uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_set_prefix(rpl_dag_t *dag, uip_ipaddr_t *prefix, unsigned len)
{
  rpl_prefix_t last_prefix;
  uint8_t last_len = dag->prefix_info.length;

  if(len > 128) {
    return 0;
  }
  if(dag->prefix_info.length != 0) {
    memcpy(&last_prefix, &dag->prefix_info, sizeof(rpl_prefix_t));
  }
  memset(&dag->prefix_info.prefix, 0, sizeof(dag->prefix_info.prefix));
  memcpy(&dag->prefix_info.prefix, prefix, (len + 7) / 8);
  dag->prefix_info.length = len;
  dag->prefix_info.flags = UIP_ND6_RA_FLAG_AUTONOMOUS;
  dag->prefix_info.lifetime = RPL_ROUTE_INFINITE_LIFETIME;
  LOG_INFO("Prefix set - will announce this in DIOs\n");
  if(dag->rank != ROOT_RANK(dag->instance)) {
    /* Autoconfigure an address if this node does not already have an address
       with this prefix. Otherwise, update the prefix. */
    if(last_len == 0) {
      LOG_INFO("rpl_set_prefix - prefix NULL\n");
      check_prefix(NULL, &dag->prefix_info);
    } else {
      LOG_INFO("rpl_set_prefix - prefix NON-NULL\n");
      check_prefix(&last_prefix, &dag->prefix_info);
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int
rpl_set_default_route(rpl_instance_t *instance, uip_ipaddr_t *from)
{
  if(instance->def_route != NULL) {
    LOG_DBG("Removing default route through ");
    LOG_DBG_6ADDR(&instance->def_route->ipaddr);
    LOG_DBG_("\n");
    uip_ds6_defrt_rm(instance->def_route);
    instance->def_route = NULL;
  }

  if(from != NULL) {
    LOG_DBG("Adding default route through ");
    LOG_DBG_6ADDR(from);
    LOG_DBG_("\n");
    instance->def_route = uip_ds6_defrt_add(from,
                                            RPL_DEFAULT_ROUTE_INFINITE_LIFETIME ? 0 : RPL_LIFETIME(instance, instance->default_lifetime));
    if(instance->def_route == NULL) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
rpl_instance_t *
rpl_alloc_instance(uint8_t instance_id)
{
  rpl_instance_t *instance, *end;

  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES;
      instance < end; ++instance) {
    if(instance->used == 0) {
      memset(instance, 0, sizeof(*instance));
      instance->instance_id = instance_id;
      instance->def_route = NULL;
      instance->used = 1;
#if RPL_WITH_PROBING
      rpl_schedule_probing(instance);
#endif /* RPL_WITH_PROBING */
      return instance;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_alloc_dag(uint8_t instance_id, uip_ipaddr_t *dag_id)
{
  rpl_dag_t *dag, *end;
  rpl_instance_t *instance;

  instance = rpl_get_instance(instance_id);
  if(instance == NULL) {
    instance = rpl_alloc_instance(instance_id);
    if(instance == NULL) {
      RPL_STAT(rpl_stats.mem_overflows++);
      return NULL;
    }
  }

  for(dag = &instance->dag_table[0], end = dag + RPL_MAX_DAG_PER_INSTANCE; dag < end; ++dag) {
    if(!dag->used) {
      memset(dag, 0, sizeof(*dag));
      dag->used = 1;
      dag->rank = RPL_INFINITE_RANK;
      dag->min_rank = RPL_INFINITE_RANK;
      dag->instance = instance;
      return dag;
    }
  }

  RPL_STAT(rpl_stats.mem_overflows++);
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
rpl_set_default_instance(rpl_instance_t *instance)
{
  default_instance = instance;
}
/*---------------------------------------------------------------------------*/
rpl_instance_t *
rpl_get_default_instance(void)
{
  return default_instance;
}
/*---------------------------------------------------------------------------*/
void
rpl_free_instance(rpl_instance_t *instance)
{
  rpl_dag_t *dag;
  rpl_dag_t *end;

  LOG_INFO("Leaving the instance %u\n", instance->instance_id);

  /* Remove any DAG inside this instance */
  for(dag = &instance->dag_table[0], end = dag + RPL_MAX_DAG_PER_INSTANCE;
      dag < end;
      ++dag) {
    if(dag->used) {
      rpl_free_dag(dag);
    }
  }

  rpl_set_default_route(instance, NULL);

#if RPL_WITH_PROBING
  ctimer_stop(&instance->probing_timer);
#endif /* RPL_WITH_PROBING */
  ctimer_stop(&instance->dio_timer);
  ctimer_stop(&instance->dao_timer);
  ctimer_stop(&instance->dao_lifetime_timer);

  if(default_instance == instance) {
    default_instance = NULL;
  }

  instance->used = 0;
}
/*---------------------------------------------------------------------------*/
void
rpl_free_dag(rpl_dag_t *dag)
{
  if(dag->joined) {
    LOG_INFO("Leaving the DAG ");
    LOG_INFO_6ADDR(&dag->dag_id);
    LOG_INFO_("\n");
    dag->joined = 0;

    /* Remove routes installed by DAOs. */
    if(RPL_IS_STORING(dag->instance)) {
      rpl_remove_routes(dag);
    }
    /* Stop the DAO retransmit timer. */
#if RPL_WITH_DAO_ACK
    ctimer_stop(&dag->instance->dao_retransmit_timer);
#endif /* RPL_WITH_DAO_ACK */

    /* Remove autoconfigured address. */
    if((dag->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS)) {
      check_prefix(&dag->prefix_info, NULL);
    }

    remove_parents(dag, 0);
  }
  dag->used = 0;
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_add_parent(rpl_dag_t *dag, rpl_dio_t *dio, uip_ipaddr_t *addr)
{
  rpl_parent_t *p = NULL;
  /* Is the parent known by DS6? Drop this request if not. Typically,
     the parent is added upon receiving a DIO. */
  const uip_lladdr_t *lladdr = uip_ds6_nbr_lladdr_from_ipaddr(addr);

  LOG_DBG("rpl_add_parent lladdr %p ", lladdr);
  LOG_DBG_6ADDR(addr);
  LOG_DBG_("\n");
  if(lladdr != NULL) {
    /* Add the parent in rpl_parents -- again this is due to DIO. */
    p = nbr_table_add_lladdr(rpl_parents, (linkaddr_t *)lladdr,
                             NBR_TABLE_REASON_RPL_DIO, dio);
    if(p == NULL) {
      LOG_DBG("rpl_add_parent p NULL\n");
    } else {
      p->dag = dag;
      p->rank = dio->rank;
      p->dtsn = dio->dtsn;
#if RPL_WITH_MC
      memcpy(&p->mc, &dio->mc, sizeof(p->mc));
#endif /* RPL_WITH_MC */
    }
  }

  return p;
}
/*---------------------------------------------------------------------------*/
static rpl_parent_t *
find_parent_any_dag_any_instance(uip_ipaddr_t *addr)
{
  uip_ds6_nbr_t *ds6_nbr = uip_ds6_nbr_lookup(addr);
  const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(ds6_nbr);
  return nbr_table_get_from_lladdr(rpl_parents, (linkaddr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_find_parent(rpl_dag_t *dag, uip_ipaddr_t *addr)
{
  rpl_parent_t *p = find_parent_any_dag_any_instance(addr);
  if(p != NULL && p->dag == dag) {
    return p;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static rpl_dag_t *
find_parent_dag(rpl_instance_t *instance, uip_ipaddr_t *addr)
{
  rpl_parent_t *p = find_parent_any_dag_any_instance(addr);
  if(p != NULL) {
    return p->dag;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_find_parent_any_dag(rpl_instance_t *instance, uip_ipaddr_t *addr)
{
  rpl_parent_t *p = find_parent_any_dag_any_instance(addr);
  if(p && p->dag && p->dag->instance == instance) {
    return p;
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_select_dag(rpl_instance_t *instance, rpl_parent_t *p)
{
  rpl_parent_t *last_parent;
  rpl_dag_t *dag, *end, *best_dag;
  rpl_rank_t old_rank;

  old_rank = instance->current_dag->rank;
  last_parent = instance->current_dag->preferred_parent;

  if(instance->current_dag->rank != ROOT_RANK(instance)) {
    rpl_select_parent(p->dag);
  }

  best_dag = NULL;
  for(dag = &instance->dag_table[0], end = dag + RPL_MAX_DAG_PER_INSTANCE;
      dag < end;
      ++dag) {
    if(dag->used && dag->preferred_parent != NULL &&
       dag->preferred_parent->rank != RPL_INFINITE_RANK) {
      if(best_dag == NULL) {
        best_dag = dag;
      } else {
        best_dag = instance->of->best_dag(best_dag, dag);
      }
    }
  }

  if(best_dag == NULL) {
    /* No parent found: the calling function needs to handle this problem. */
    return NULL;
  }

  if(instance->current_dag != best_dag) {
    /* Remove routes installed by DAOs. */
    if(RPL_IS_STORING(instance)) {
      rpl_remove_routes(instance->current_dag);
    }

    LOG_INFO("New preferred DAG: ");
    LOG_INFO_6ADDR(&best_dag->dag_id);
    LOG_INFO_("\n");

    if(best_dag->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
      check_prefix(&instance->current_dag->prefix_info, &best_dag->prefix_info);
    } else if(instance->current_dag->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
      check_prefix(&instance->current_dag->prefix_info, NULL);
    }

    best_dag->joined = 1;
    instance->current_dag->joined = 0;
    instance->current_dag = best_dag;
  }

  instance->of->update_metric_container(instance);
  /* Update the DAG rank. */
  best_dag->rank = rpl_rank_via_parent(best_dag->preferred_parent);
  if(last_parent == NULL || best_dag->rank < best_dag->min_rank) {
    /*
     * This is a slight departure from RFC6550: if we had no preferred
     * parent before, reset min_rank. This helps recovering from
     * temporary bad link conditions.
     */
    best_dag->min_rank = best_dag->rank;
  }

  if(!acceptable_rank(best_dag, best_dag->rank)) {
    LOG_WARN("New rank unacceptable!\n");
    rpl_set_preferred_parent(instance->current_dag, NULL);
    if(RPL_IS_STORING(instance) && last_parent != NULL) {
      /* Send a No-Path DAO to the removed preferred parent. */
      dao_output(last_parent, RPL_ZERO_LIFETIME);
    }
    return NULL;
  }

  if(best_dag->preferred_parent != last_parent) {
    rpl_set_default_route(instance, rpl_parent_get_ipaddr(best_dag->preferred_parent));
    LOG_INFO("Changed preferred parent, rank changed from %u to %u\n",
             (unsigned)old_rank, best_dag->rank);
    RPL_STAT(rpl_stats.parent_switch++);
    if(RPL_IS_STORING(instance)) {
      if(last_parent != NULL) {
        /* Send a No-Path DAO to the removed preferred parent. */
        dao_output(last_parent, RPL_ZERO_LIFETIME);
      }
      /* Trigger DAO transmission from immediate children.
       * Only for storing mode, see RFC6550 section 9.6. */
      RPL_LOLLIPOP_INCREMENT(instance->dtsn_out);
    }
    /* The DAO parent set changed -- schedule a DAO transmission. If
       MOP = MOP0, we do not want downward routes. */
    if(instance->mop != RPL_MOP_NO_DOWNWARD_ROUTES) {
      rpl_schedule_dao(instance);
    }

    rpl_reset_dio_timer(instance);
    if(LOG_DBG_ENABLED) {
      rpl_print_neighbor_list();
    }
  } else if(best_dag->rank != old_rank) {
    LOG_DBG("RPL: Preferred parent update, rank changed from %u to %u\n",
            (unsigned)old_rank, best_dag->rank);
  }
  return best_dag;
}
/*---------------------------------------------------------------------------*/
static int
filter_parent(rpl_parent_t * p, rpl_dag_t *dag, int fresh_only)
{
  /* Exclude parents that are from other DAGs or are announcing an
     infinite rank. */
  if(p->dag != dag || p->rank == RPL_INFINITE_RANK ||
     p->rank < ROOT_RANK(dag->instance)) {
    if(p->rank < ROOT_RANK(dag->instance)) {
      LOG_WARN("Parent has invalid rank\n");
    }
    return 1;
  }

  if(fresh_only && !rpl_parent_is_fresh(p)) {
    /* Filter out non-fresh parents if fresh_only is set. */
    return 1;
  }

#if UIP_ND6_SEND_NS
  /* Exclude links to a neighbor that is not reachable at a NUD level. */
  if(rpl_get_nbr(p) == NULL) {
    return 1;
  }
#endif /* UIP_ND6_SEND_NS */

  return 0;
}
/*---------------------------------------------------------------------------*/
static rpl_parent_t *
best_parent(rpl_dag_t *dag, int fresh_only)
{
  rpl_parent_t *p;
  rpl_of_t *of;
  rpl_parent_t *best = NULL;

  if(dag == NULL || dag->instance == NULL || dag->instance->of == NULL) {
    return NULL;
  }

  of = dag->instance->of;

#if RPL_WITH_PMAOF
  // Maintain the stability of the preferred parent if performance is acceptable.
  int pp_is_acceptable = 0;
  if(dag->preferred_parent != NULL &&
     !filter_parent(dag->preferred_parent, dag, fresh_only)) {
    pp_is_acceptable = !of->parent_is_acceptable || of->parent_is_acceptable(dag->preferred_parent);
    if(pp_is_acceptable) {
      return dag->preferred_parent;
    }
  }
#endif

  /* Search for the best parent according to the OF */
  for(p = nbr_table_head(rpl_parents); p != NULL; p = nbr_table_next(rpl_parents, p)) {

    if(filter_parent(p, dag, fresh_only)) {
      continue;
    }

    /* Now we have an acceptable parent, check if it is the new best. */
    best = of->best_parent(best, p);
  }

  return best;
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_select_parent(rpl_dag_t *dag)
{
  /* Look for best parent (regardless of freshness) */
  rpl_parent_t *best = best_parent(dag, 0);

  if(best != NULL) {
#if RPL_WITH_PROBING
    if(rpl_parent_is_fresh(best)) {
      rpl_set_preferred_parent(dag, best);
      /* Unschedule any already scheduled urgent probing. */
      dag->instance->urgent_probing_target = NULL;
    } else {
      /* The best is not fresh. Look for the best fresh now. */
      rpl_parent_t *best_fresh = best_parent(dag, 1);
      if(best_fresh == NULL) {
        /* No fresh parent around, use best (non-fresh). */
        rpl_set_preferred_parent(dag, best);
      } else {
        /* Use best fresh */
        rpl_set_preferred_parent(dag, best_fresh);
      }
#if RPL_WITH_PMAOF
      if(!rpl_parent_probe_recent(best)) {
        /* Probe the best parent shortly in order to get a fresh estimate. */
        dag->instance->urgent_probing_target = best;
        rpl_schedule_probing_now(dag->instance);
      }
#else
      /* Probe the best parent shortly in order to get a fresh estimate. */
      dag->instance->urgent_probing_target = best;
      rpl_schedule_probing_now(dag->instance);
#endif
    }
#else /* RPL_WITH_PROBING */
    rpl_set_preferred_parent(dag, best);
    dag->rank = rpl_rank_via_parent(dag->preferred_parent);
#endif /* RPL_WITH_PROBING */
  } else {
    rpl_set_preferred_parent(dag, NULL);
  }

  dag->rank = rpl_rank_via_parent(dag->preferred_parent);
  return dag->preferred_parent;
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_parent(rpl_parent_t *parent)
{
  LOG_INFO("Removing parent ");
  LOG_INFO_6ADDR(rpl_parent_get_ipaddr(parent));
  LOG_INFO_("\n");

  rpl_nullify_parent(parent);

  nbr_table_remove(rpl_parents, parent);
}
/*---------------------------------------------------------------------------*/
void
rpl_nullify_parent(rpl_parent_t *parent)
{
  rpl_dag_t *dag = parent->dag;
  /*
   * This function can be called when the preferred parent is NULL, so
   * we need to handle this condition in order to trigger uip_ds6_defrt_rm.
   */
  if(parent == dag->preferred_parent || dag->preferred_parent == NULL) {
    dag->rank = RPL_INFINITE_RANK;
    if(dag->joined) {
      if(dag->instance->def_route != NULL) {
        LOG_DBG("Removing default route ");
        LOG_DBG_6ADDR(rpl_parent_get_ipaddr(parent));
        LOG_DBG_("\n");
        uip_ds6_defrt_rm(dag->instance->def_route);
        dag->instance->def_route = NULL;
      }
      /* Send a No-Path DAO only when nullifying preferred parent. */
      if(parent == dag->preferred_parent) {
        if(RPL_IS_STORING(dag->instance)) {
          dao_output(parent, RPL_ZERO_LIFETIME);
        }
        rpl_set_preferred_parent(dag, NULL);
      }
    }
  }

  LOG_INFO("Nullifying parent ");
  LOG_INFO_6ADDR(rpl_parent_get_ipaddr(parent));
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
void
rpl_move_parent(rpl_dag_t *dag_src, rpl_dag_t *dag_dst, rpl_parent_t *parent)
{
  if(parent == dag_src->preferred_parent) {
    rpl_set_preferred_parent(dag_src, NULL);
    dag_src->rank = RPL_INFINITE_RANK;
    if(dag_src->joined && dag_src->instance->def_route != NULL) {
      LOG_DBG("Removing default route ");
      LOG_DBG_6ADDR(rpl_parent_get_ipaddr(parent));
      LOG_DBG_("\n");
      LOG_DBG("rpl_move_parent\n");
      uip_ds6_defrt_rm(dag_src->instance->def_route);
      dag_src->instance->def_route = NULL;
    }
  } else if(dag_src->joined) {
    if(RPL_IS_STORING(dag_src->instance)) {
      /* Remove uIPv6 routes that have this parent as the next hop. */
      rpl_remove_routes_by_nexthop(rpl_parent_get_ipaddr(parent), dag_src);
    }
  }

  LOG_INFO("Moving parent ");
  LOG_INFO_6ADDR(rpl_parent_get_ipaddr(parent));
  LOG_INFO_("\n");

  parent->dag = dag_dst;
}
/*---------------------------------------------------------------------------*/
static rpl_dag_t *
rpl_get_any_dag_with_parent(bool requires_parent)
{
  int i;

  for(i = 0; i < RPL_MAX_INSTANCES; ++i) {
    if(instance_table[i].used
       && instance_table[i].current_dag->joined
       && (!requires_parent || instance_table[i].current_dag->preferred_parent != NULL)) {
      return instance_table[i].current_dag;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
rpl_has_joined(void)
{
  if(rpl_dag_root_is_root()) {
    return 1;
  }
  return rpl_get_any_dag_with_parent(true) != NULL;
}
/*---------------------------------------------------------------------------*/
int
rpl_has_downward_route(void)
{
  int i;
  if(rpl_dag_root_is_root()) {
    return 1; /* We are the root, and know the route to ourself */
  }
  for(i = 0; i < RPL_MAX_INSTANCES; ++i) {
    if(instance_table[i].used && instance_table[i].has_downward_route) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_get_dag(const uip_ipaddr_t *addr)
{
  for(int i = 0; i < RPL_MAX_INSTANCES; ++i) {
    if(instance_table[i].used) {
      for(int j = 0; j < RPL_MAX_DAG_PER_INSTANCE; ++j) {
        if(instance_table[i].dag_table[j].joined
           && uip_ipaddr_prefixcmp(&instance_table[i].dag_table[j].dag_id, addr,
                                   instance_table[i].dag_table[j].prefix_info.length)) {
          return &instance_table[i].dag_table[j];
        }
      }
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_get_any_dag(void)
{
  return rpl_get_any_dag_with_parent(false);
}
/*---------------------------------------------------------------------------*/
rpl_instance_t *
rpl_get_instance(uint8_t instance_id)
{
  int i;

  for(i = 0; i < RPL_MAX_INSTANCES; ++i) {
    if(instance_table[i].used && instance_table[i].instance_id == instance_id) {
      return &instance_table[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
rpl_of_t *
rpl_find_of(rpl_ocp_t ocp)
{
  for(unsigned i = 0;
      i < sizeof(objective_functions) / sizeof(objective_functions[0]);
      i++) {
    if(objective_functions[i]->ocp == ocp) {
      return objective_functions[i];
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
void
rpl_join_instance(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  rpl_instance_t *instance;
  rpl_dag_t *dag;
  rpl_parent_t *p;
  rpl_of_t *of;

  if((!RPL_WITH_NON_STORING && dio->mop == RPL_MOP_NON_STORING)
     || (!RPL_WITH_STORING && (dio->mop == RPL_MOP_STORING_NO_MULTICAST
                               || dio->mop == RPL_MOP_STORING_MULTICAST))) {
    LOG_WARN("DIO advertising a non-supported MOP %u\n", dio->mop);
    return;
  }

  /* Determine the objective function by using the objective code
     point of the DIO. */
  of = rpl_find_of(dio->ocp);
  if(of == NULL) {
    LOG_WARN("DIO for DAG instance %u does not specify a supported OF: %u\n",
             dio->instance_id, dio->ocp);
    return;
  }

  dag = rpl_alloc_dag(dio->instance_id, &dio->dag_id);
  if(dag == NULL) {
    LOG_ERR("Failed to allocate a DAG object!\n");
    return;
  }

  instance = dag->instance;

  p = rpl_add_parent(dag, dio, from);
  LOG_DBG("Adding ");
  LOG_DBG_6ADDR(from);
  LOG_DBG_(" as a parent: ");
  if(p == NULL) {
    LOG_DBG_("failed\n");
    instance->used = 0;
    return;
  }
  p->dtsn = dio->dtsn;
  LOG_DBG_("succeeded\n");

  /* Autoconfigure an address if this node does not already have an
     address with this prefix. */
  if(dio->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
    check_prefix(NULL, &dio->prefix_info);
  }

  dag->joined = 1;
  dag->preference = dio->preference;
  dag->grounded = dio->grounded;
  dag->version = dio->version;

  instance->of = of;
  instance->mop = dio->mop;
  instance->mc.type = dio->mc.type;
  instance->mc.flags = dio->mc.flags;
  instance->mc.aggr = dio->mc.aggr;
  instance->mc.prec = dio->mc.prec;
  instance->current_dag = dag;
  instance->dtsn_out = RPL_LOLLIPOP_INIT;

  instance->max_rankinc = dio->dag_max_rankinc;
  instance->min_hoprankinc = dio->dag_min_hoprankinc;
  instance->dio_intdoubl = dio->dag_intdoubl;
  instance->dio_intmin = dio->dag_intmin;
  instance->dio_intcurrent = instance->dio_intmin + instance->dio_intdoubl;
  instance->dio_redundancy = dio->dag_redund;
  instance->default_lifetime = dio->default_lifetime;
  instance->lifetime_unit = dio->lifetime_unit;

  memcpy(&dag->dag_id, &dio->dag_id, sizeof(dio->dag_id));

  /* Copy prefix information from the DIO into the DAG object. */
  memcpy(&dag->prefix_info, &dio->prefix_info, sizeof(rpl_prefix_t));

  rpl_set_preferred_parent(dag, p);
  instance->of->update_metric_container(instance);
  dag->rank = rpl_rank_via_parent(p);
  /* So far this is the lowest rank we are aware of. */
  dag->min_rank = dag->rank;

  if(default_instance == NULL) {
    default_instance = instance;
  }

  LOG_INFO("Joined DAG with instance ID %u, rank %hu, DAG ID ",
           dio->instance_id, dag->rank);
  LOG_INFO_6ADDR(&dag->dag_id);
  LOG_INFO_("\n");

  LOG_ANNOTATE("#A join=%u\n", dag->dag_id.u8[sizeof(dag->dag_id) - 1]);

  rpl_reset_dio_timer(instance);
  rpl_set_default_route(instance, from);

  if(instance->mop != RPL_MOP_NO_DOWNWARD_ROUTES) {
    rpl_schedule_dao(instance);
  } else {
    LOG_WARN("The DIO does not meet the prerequisites for sending a DAO\n");
  }

  instance->of->reset(dag);
}
#if RPL_MAX_DAG_PER_INSTANCE > 1
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_add_dag(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  rpl_instance_t *instance;
  rpl_dag_t *dag, *previous_dag;
  rpl_parent_t *p;
  rpl_of_t *of;

  dag = rpl_alloc_dag(dio->instance_id, &dio->dag_id);
  if(dag == NULL) {
    LOG_ERR("Failed to allocate a DAG object!\n");
    return NULL;
  }

  instance = dag->instance;

  previous_dag = find_parent_dag(instance, from);
  if(previous_dag == NULL) {
    LOG_DBG("Adding ");
    LOG_DBG_6ADDR(from);
    LOG_DBG_(" as a parent: ");
    p = rpl_add_parent(dag, dio, from);
    if(p == NULL) {
      LOG_DBG_("failed\n");
      dag->used = 0;
      return NULL;
    }
    LOG_DBG_("succeeded\n");
  } else {
    p = rpl_find_parent(previous_dag, from);
    if(p != NULL) {
      rpl_move_parent(previous_dag, dag, p);
    }
  }
  p->rank = dio->rank;

  /* Determine the objective function by using the objective code
     point of the DIO. */
  of = rpl_find_of(dio->ocp);
  if(of != instance->of ||
     instance->mop != dio->mop ||
     instance->max_rankinc != dio->dag_max_rankinc ||
     instance->min_hoprankinc != dio->dag_min_hoprankinc ||
     instance->dio_intdoubl != dio->dag_intdoubl ||
     instance->dio_intmin != dio->dag_intmin ||
     instance->dio_redundancy != dio->dag_redund ||
     instance->default_lifetime != dio->default_lifetime ||
     instance->lifetime_unit != dio->lifetime_unit) {
    LOG_WARN("DIO for DAG instance %u incompatible with previous DIO\n",
             dio->instance_id);
    rpl_remove_parent(p);
    dag->used = 0;
    return NULL;
  }

  dag->used = 1;
  dag->grounded = dio->grounded;
  dag->preference = dio->preference;
  dag->version = dio->version;

  memcpy(&dag->dag_id, &dio->dag_id, sizeof(dio->dag_id));

  /* copy prefix information into the DAG. */
  memcpy(&dag->prefix_info, &dio->prefix_info, sizeof(rpl_prefix_t));

  rpl_set_preferred_parent(dag, p);
  dag->rank = rpl_rank_via_parent(p);
  dag->min_rank = dag->rank; /* So far this is the lowest rank we know of. */

  LOG_INFO("Joined DAG with instance ID %u, rank %hu, DAG ID ",
           dio->instance_id, dag->rank);
  LOG_INFO_6ADDR(&dag->dag_id);
  LOG_INFO_("\n");

  LOG_ANNOTATE("#A join=%u\n", dag->dag_id.u8[sizeof(dag->dag_id) - 1]);

  rpl_process_parent_event(instance, p);
  p->dtsn = dio->dtsn;

  return dag;
}
#endif /* RPL_MAX_DAG_PER_INSTANCE > 1 */

/*---------------------------------------------------------------------------*/
static void
global_repair(uip_ipaddr_t *from, rpl_dag_t *dag, rpl_dio_t *dio)
{
  rpl_parent_t *p;

  remove_parents(dag, 0);
  dag->version = dio->version;

  /* Copy parts of the configuration so that it propagates in the network. */
  dag->instance->dio_intdoubl = dio->dag_intdoubl;
  dag->instance->dio_intmin = dio->dag_intmin;
  dag->instance->dio_redundancy = dio->dag_redund;
  dag->instance->default_lifetime = dio->default_lifetime;
  dag->instance->lifetime_unit = dio->lifetime_unit;

  dag->instance->of->reset(dag);
  dag->min_rank = RPL_INFINITE_RANK;
  RPL_LOLLIPOP_INCREMENT(dag->instance->dtsn_out);

  p = rpl_add_parent(dag, dio, from);
  if(p == NULL) {
    LOG_ERR("Failed to add a parent during the global repair\n");
    dag->rank = RPL_INFINITE_RANK;
  } else {
    dag->rank = rpl_rank_via_parent(p);
    dag->min_rank = dag->rank;
    LOG_DBG("rpl_process_parent_event global repair\n");
    rpl_process_parent_event(dag->instance, p);
  }

  LOG_DBG("Participating in a global repair (version=%u, rank=%hu)\n",
          dag->version, dag->rank);

  RPL_STAT(rpl_stats.global_repairs++);
}
/*---------------------------------------------------------------------------*/
void
rpl_local_repair(rpl_instance_t *instance)
{
  int i;

  if(instance == NULL) {
    LOG_WARN("local repair requested for instance NULL\n");
    return;
  }
  LOG_INFO("Starting a local instance repair\n");
  for(i = 0; i < RPL_MAX_DAG_PER_INSTANCE; i++) {
    if(instance->dag_table[i].used) {
      instance->dag_table[i].rank = RPL_INFINITE_RANK;
      nullify_parents(&instance->dag_table[i], 0);
    }
  }

  /* No downward route anymore. */
  instance->has_downward_route = 0;
#if RPL_WITH_DAO_ACK
  ctimer_stop(&instance->dao_retransmit_timer);
#endif /* RPL_WITH_DAO_ACK */

  rpl_reset_dio_timer(instance);
  if(RPL_IS_STORING(instance)) {
    /*
     * Request refresh of DAO registrations next DIO. This is only for
     * storing mode. In non-storing mode, non-root nodes increment
     * DTSN only on when their parent do, or on global repair (see
     * RFC6550 section 9.6.)
     */
    RPL_LOLLIPOP_INCREMENT(instance->dtsn_out);
  }

  RPL_STAT(rpl_stats.local_repairs++);
}
/*---------------------------------------------------------------------------*/
void
rpl_recalculate_ranks(void)
{
  rpl_parent_t *p;

  /*
   * We recalculate ranks when we receive feedback from the system rather
   * than RPL protocol messages. This periodical recalculation is called
   * from a timer in order to keep the stack depth reasonably low.
   */
  p = nbr_table_head(rpl_parents);
  while(p != NULL) {
    if(p->dag != NULL && p->dag->instance && (p->flags & RPL_PARENT_FLAG_UPDATED)) {
      p->flags &= ~RPL_PARENT_FLAG_UPDATED;
      LOG_DBG("rpl_process_parent_event recalculate_ranks\n");
      if(!rpl_process_parent_event(p->dag->instance, p)) {
        LOG_DBG("A parent was dropped\n");
      }
    }
    p = nbr_table_next(rpl_parents, p);
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_process_parent_event(rpl_instance_t *instance, rpl_parent_t *p)
{
  int return_value;
  rpl_parent_t *last_parent = instance->current_dag->preferred_parent;

#if LOG_DBG_ENABLED
  rpl_rank_t old_rank;
  old_rank = instance->current_dag->rank;
#endif /* LOG_DBG_ENABLED */

  return_value = 1;

  if(RPL_IS_STORING(instance)
     && uip_ds6_route_is_nexthop(rpl_parent_get_ipaddr(p))
     && !rpl_parent_is_reachable(p) && instance->mop > RPL_MOP_NON_STORING) {
    LOG_WARN("Unacceptable link %u, removing routes via: ",
             rpl_get_parent_link_metric(p));
    LOG_WARN_6ADDR(rpl_parent_get_ipaddr(p));
    LOG_WARN_("\n");
    rpl_remove_routes_by_nexthop(rpl_parent_get_ipaddr(p), p->dag);
  }

  rpl_rank_t p_rank = rpl_rank_via_parent(p);
  if(!acceptable_rank(p->dag, p_rank)) {
    /* The candidate parent is no longer valid: the rank increase
       resulting from the choice of it as a parent would be too high. */
    LOG_WARN("Unacceptable rank (Parent rank %u, Rank via parent %u, Current min %u, MaxRankInc %u)\n",
             (unsigned)p->rank, (unsigned)p_rank,
             p->dag->min_rank, p->dag->instance->max_rankinc);
    rpl_nullify_parent(p);
    if(p != instance->current_dag->preferred_parent) {
      return 0;
    } else {
      return_value = 0;
    }
  }

  if(rpl_select_dag(instance, p) == NULL) {
    if(last_parent != NULL) {
      /* No suitable parent anymore; trigger a local repair. */
      LOG_ERR("No parents found in any DAG\n");
      rpl_local_repair(instance);
      return 0;
    }
  }

#if LOG_DBG_ENABLED
  if(DAG_RANK(old_rank, instance) !=
     DAG_RANK(instance->current_dag->rank, instance)) {
    LOG_INFO("Moving in the instance from rank %hu to %hu\n",
             DAG_RANK(old_rank, instance),
             DAG_RANK(instance->current_dag->rank, instance));
    if(instance->current_dag->rank != RPL_INFINITE_RANK) {
      LOG_DBG("The preferred parent is ");
      LOG_DBG_6ADDR(rpl_parent_get_ipaddr(instance->current_dag->preferred_parent));
      LOG_DBG_(" (rank %u)\n",
               (unsigned)DAG_RANK(instance->current_dag->preferred_parent->rank,
                                  instance));
    } else {
      LOG_WARN("We don't have any parent");
    }
  }
#endif /* LOG_DBG_ENABLED */

  return return_value;
}
/*---------------------------------------------------------------------------*/
static int
add_nbr_from_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  /* Add this to the neighbor cache if not already there. */
  if(rpl_icmp6_update_nbr_table(from, NBR_TABLE_REASON_RPL_DIO, dio) == NULL) {
    LOG_ERR("Out of memory, dropping DIO from ");
    LOG_ERR_6ADDR(from);
    LOG_ERR_("\n");
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  rpl_instance_t *instance;
  rpl_dag_t *dag, *previous_dag;
  rpl_parent_t *p;

#if RPL_WITH_MULTICAST
  /*
   * If the root is advertising MOP 2, but we support MOP 3, we can
   * still join. In that scenario, we suppress DAOs for multicast
   * targets.
   */
  if(dio->mop < RPL_MOP_STORING_NO_MULTICAST) {
#else
  if(dio->mop != RPL_MOP_DEFAULT) {
#endif
    LOG_ERR("Ignoring a DIO with an unsupported MOP: %d\n", dio->mop);
    return;
  }

  dag = get_dag(dio->instance_id, &dio->dag_id);
  instance = rpl_get_instance(dio->instance_id);

  if(dag != NULL && instance != NULL) {
    if(lollipop_greater_than(dio->version, dag->version)) {
      if(dag->rank == ROOT_RANK(instance)) {
        LOG_WARN("Root received inconsistent DIO version number (current: %u, received: %u)\n",
                 dag->version, dio->version);
        dag->version = dio->version;
        RPL_LOLLIPOP_INCREMENT(dag->version);
        rpl_reset_dio_timer(instance);
      } else {
        LOG_DBG("Global repair\n");
        if(dio->prefix_info.length != 0) {
          if(dio->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
            LOG_DBG("Prefix announced in DIO\n");
            rpl_set_prefix(dag, &dio->prefix_info.prefix,
                           dio->prefix_info.length);
          }
        }
        global_repair(from, dag, dio);
      }
      return;
    }

    if(lollipop_greater_than(dag->version, dio->version)) {
      /* The DIO sender is on an older version of the DAG. */
      LOG_WARN("old version received => inconsistency detected\n");
      if(dag->joined) {
        rpl_reset_dio_timer(instance);
        return;
      }
    }
  }

  if(instance == NULL) {
    LOG_INFO("New instance detected (ID=%u): Joining...\n", dio->instance_id);
    if(add_nbr_from_dio(from, dio)) {
      rpl_join_instance(from, dio);
    } else {
      LOG_WARN("Not joining since we could not add a parent\n");
    }
    return;
  }

  if(instance->current_dag->rank == ROOT_RANK(instance) &&
     instance->current_dag != dag) {
    LOG_WARN("Root ignored DIO for different DAG\n");
    return;
  }

  if(dag == NULL) {
#if RPL_MAX_DAG_PER_INSTANCE > 1
    LOG_INFO("Adding new DAG to known instance.\n");
    if(!add_nbr_from_dio(from, dio)) {
      LOG_WARN("Could not add new DAG, could not add parent\n");
      return;
    }
    dag = rpl_add_dag(from, dio);
    if(dag == NULL) {
      LOG_WARN("Failed to add DAG.\n");
      return;
    }
#else /* RPL_MAX_DAG_PER_INSTANCE > 1 */
    LOG_WARN("Only one instance supported.\n");
    return;
#endif /* RPL_MAX_DAG_PER_INSTANCE > 1 */
  }

  if(dio->rank < ROOT_RANK(instance)) {
    LOG_INFO("Ignoring DIO with too low rank: %u\n",
             (unsigned)dio->rank);
    return;
  }

  /* Prefix Information Option included to add a new prefix. */
  if(dio->prefix_info.length != 0) {
    if(dio->prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS) {
      LOG_DBG("Prefix announced in DIO\n");
      rpl_set_prefix(dag, &dio->prefix_info.prefix, dio->prefix_info.length);
    }
  }

  if(!add_nbr_from_dio(from, dio)) {
    LOG_WARN("Could not add parent based on DIO\n");
    return;
  }

  if(dag->rank == ROOT_RANK(instance)) {
    if(dio->rank != RPL_INFINITE_RANK) {
      instance->dio_counter++;
    }
    return;
  }

  /* The DIO comes from a valid DAG, so we can refresh its lifetime. */
  dag->lifetime = (1UL << (instance->dio_intmin + instance->dio_intdoubl)) *
    RPL_DAG_LIFETIME / 1000;
  LOG_INFO("Set DAG ");
  LOG_INFO_6ADDR(&dag->dag_id);
  LOG_INFO_(" lifetime to %ld\n", (long int)dag->lifetime);

  /*
   * At this point, we know that this DIO pertains to a DAG that we
   * are already part of. We consider the sender of the DIO to be a
   * candidate parent, and let rpl_process_parent_event decide whether
   * to keep it in the set.
   */

  p = rpl_find_parent(dag, from);
  if(p == NULL) {
    previous_dag = find_parent_dag(instance, from);
    if(previous_dag == NULL) {
      /* Add the DIO sender as a candidate parent. */
      p = rpl_add_parent(dag, dio, from);
      if(p == NULL) {
        LOG_WARN("Failed to add a new parent (");
        LOG_WARN_6ADDR(from);
        LOG_WARN_(")\n");
        return;
      }
      LOG_INFO("New candidate parent with rank %u: ", (unsigned)p->rank);
      LOG_INFO_6ADDR(from);
      LOG_INFO_("\n");
    } else {
      p = rpl_find_parent(previous_dag, from);
      if(p != NULL) {
        rpl_move_parent(previous_dag, dag, p);
      }
    }
  } else {
    if(p->rank == dio->rank) {
      LOG_INFO("Received consistent DIO\n");
      if(dag->joined) {
        instance->dio_counter++;
      }
    }
  }
  p->rank = dio->rank;

  if(dio->rank == RPL_INFINITE_RANK && p == dag->preferred_parent) {
    /* Our preferred parent advertised an infinite rank, reset DIO timer. */
    rpl_reset_dio_timer(instance);
  }

  /* Parent info has been updated, trigger rank recalculation. */
  p->flags |= RPL_PARENT_FLAG_UPDATED;

  link_stats_nbr_rssi_callback(rpl_get_parent_lladdr(p), dio->mc.obj.movfac.par_rssi, dio->mc.obj.movfac.time_since);

  LOG_INFO("preferred DAG ");
  LOG_INFO_6ADDR(&instance->current_dag->dag_id);
  LOG_INFO_(", rank %u, min_rank %u, ",
            instance->current_dag->rank, instance->current_dag->min_rank);
  LOG_INFO_("parent rank %u, link metric %u\n",
            p->rank, rpl_get_parent_link_metric(p));

  /* We have allocated a candidate parent; process the DIO further. */

#if RPL_WITH_MC
  memcpy(&p->mc, &dio->mc, sizeof(p->mc));
#endif /* RPL_WITH_MC */
  if(rpl_process_parent_event(instance, p) == 0) {
    LOG_WARN("The candidate parent is rejected\n");
    return;
  }

  /* We don't use route control, so we can have only one official parent. */
  if(dag->joined && p == dag->preferred_parent) {
    if(should_refresh_routes(instance, dio, p)) {
      /* Our parent is requesting a new DAO. Increment DTSN in turn,
         in both storing and non-storing mode (see RFC6550 section 9.6.) */
      RPL_LOLLIPOP_INCREMENT(instance->dtsn_out);
      rpl_schedule_dao(instance);
    }
    /*
     * We received a new DIO from our preferred parent.  Call
     * uip_ds6_defrt_add to set a fresh value for the lifetime
     * counter.
     */
    uip_ds6_defrt_add(from, RPL_DEFAULT_ROUTE_INFINITE_LIFETIME ? 0 : RPL_LIFETIME(instance, instance->default_lifetime));
  }
  p->dtsn = dio->dtsn;
}
/*---------------------------------------------------------------------------*/
/** @} */
