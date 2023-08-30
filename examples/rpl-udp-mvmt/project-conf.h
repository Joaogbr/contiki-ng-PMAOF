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
#define LOG_CONF_LEVEL_LS                          LOG_LEVEL_INFO
#define TSCH_LOG_CONF_PER_SLOT                     0

#if !MAC_CONF_WITH_TSCH
#define RPL_CALLBACK_PARENT_SWITCH mvmt_rpl_callback_parent_switch
#endif

/* Enable printing of packet counters */
#define LINK_STATS_CONF_PACKET_COUNTERS          1
//#define LINK_STATS_CONF_RSSI_ARR_LEN             10
#define LINK_STATS_CONF_RSSI_WITH_EMANEXT        1

/* RPL config */
#define RPL_CONF_OF_OCP RPL_OCP_MVMTOF
#define RPL_CONF_SUPPORTED_OFS {&rpl_mvmtof}
#define RPL_CONF_WITH_MC 1
#define RPL_CONF_DAG_MC RPL_DAG_MC_RSSI

/* Always larger than the link cost, merely acts as a hop count (RFC6719)*/
#define RPL_CONF_MIN_HOPRANKINC          1280
#define RPL_CONF_MAX_RANKINC             (7 * RPL_MIN_HOPRANKINC)

//#define RPL_CONF_DIO_INTERVAL_MIN        3 //12
//#define RPL_DIO_INTERVAL_DOUBLINGS       20 //8
//#define RPL_CONF_PROBING_INTERVAL        (30 * CLOCK_SECOND) //(60 * CLOCK_SECOND)
//#define RPL_CONF_DIS_INTERVAL            30 //60
//#define RPL_CONF_WITH_DAO_ACK            1 //0

/* Application settings */
#define APP_WARM_UP_PERIOD_SEC 120

#define SICSLOWPAN_CONF_FRAG 0 /* 1 if using cam, 0 otherwise */
#define UIP_CONF_BUFFER_SIZE 200 /* 300 if using cam, 200 otherwise */
//#define QUEUEBUF_CONF_NUM 8
