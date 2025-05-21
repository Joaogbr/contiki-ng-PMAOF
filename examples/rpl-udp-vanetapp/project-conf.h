/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science (EDI)
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
 * Author: Atis Elsts <atis.elsts@edi.lv>
 */

/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_COAP                        LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_LWM2M                       LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_6TOP                        LOG_LEVEL_NONE
#define LOG_CONF_LEVEL_LS                          LOG_LEVEL_INFO
#define TSCH_LOG_CONF_PER_SLOT                     0

#define PROCESS_CONF_NUMEVENTS       32

/* Enable printing of packet counters */
#define LINK_STATS_CONF_PACKET_COUNTERS          0
#define LINK_STATS_CONF_ETX_DIVISOR              128
//#define LINK_STATS_CONF_RSSI_ARR_LEN             10
#define LINK_STATS_CONF_RSSI_WITH_EMANEXT        1
#define LINK_STATS_CONF_MIN_RSSI_COUNT           3

/* Handle 16 neighbors */
#define NBR_TABLE_CONF_MAX_NEIGHBORS    150
//#define NBR_TABLE_CONF_GC_GET_WORST            rpl_nbr_gc_get_worst_path

/* Handle 16 routes    */
#define NETSTACK_MAX_ROUTE_ENTRIES      150

/* RPL config */
#define RPL_CONF_MOP RPL_MOP_NON_STORING
#define RPL_CONF_SUPPORTED_OFS {&rpl_of0, &rpl_mrhof, &rpl_pmaof}
#define RPL_CONF_WITH_PMAOF 1
#if RPL_CONF_WITH_PMAOF
#define RPL_CONF_OF_OCP RPL_OCP_PMAOF
#define RPL_CONF_WITH_MC 1
#define RPL_CONF_DAG_MC RPL_DAG_MC_SSV
#else
#define RPL_CONF_OF_OCP RPL_OCP_MRHOF
#endif

/* If always larger than the link cost, merely acts as a hop count (RFC6719)*/
//#define RPL_CONF_MIN_HOPRANKINC          200
//#define RPL_CONF_MAX_RANKINC             (10 * RPL_CONF_MIN_HOPRANKINC)

#define RPL_CONF_PROBING_SEND_FUNC(instance, addr) dis_output((addr))

#define RPL_CONF_DIO_INTERVAL_MIN 9
#define RPL_CONF_DIO_INTERVAL_DOUBLINGS 3
#define RPL_CONF_PROBING_INTERVAL        (10 * CLOCK_SECOND) //(60 * CLOCK_SECOND)
//#define RPL_CONF_DIS_INTERVAL            30 //60
//#define RPL_CONF_WITH_DAO_ACK            1 //0

#define PMAOF_CONF_CF_ALPHA      0.5f
//#define PMAOF_CONF_CF_ALPHA      5.0f /* Linear RSSI */
#define PMAOF_CONF_CF_BETA       1.0f
#define PMAOF_CONF_MAX_ABS_RSSI  95 /* dBm */
#define PMAOF_CONF_ABS_RSSI_RED  87
#define PMAOF_CONF_SSR_RED     2*PMAOF_CONF_ABS_RSSI_RED

/* Application settings */
#define APP_WARM_UP_PERIOD_SEC 120

#define SICSLOWPAN_CONF_FRAG 0 /* 1 if using cam, 0 otherwise */
//#define UIP_CONF_BUFFER_SIZE 200 /* 300 if using cam, 200 otherwise */
#define QUEUEBUF_CONF_NUM 8

// 10
#define RPL_CONF_DEFAULT_LIFETIME_UNIT       10

/*
 * Default route lifetime as a multiple of the lifetime unit.
 */
#define RPL_CONF_DEFAULT_LIFETIME            1

//#define COOJA_RADIO_CONF_BUFSIZE 200
