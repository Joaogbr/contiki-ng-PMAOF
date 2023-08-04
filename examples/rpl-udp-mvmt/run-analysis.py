#!/usr/bin/env python3

import os
import sys
import time
import numpy as np
import matplotlib.pyplot as pl

###########################################

# If set to true, all nodes are plotted, even those with no valid data
PLOT_ALL_NODES = True

###########################################

LOG_FILE = 'COOJA.testlog'

COORDINATOR_ID = 1

# for charge calculations
CC2650_MHZ = 48
CC2650_RADIO_TX_CURRENT_MA          = 9.100 # at 5 dBm, from CC2650 datasheet
CC2650_RADIO_RX_CURRENT_MA          = 5.900 # from CC2650 datasheet
CC2650_RADIO_CPU_ON_CURRENT         = 0.061 * CC2650_MHZ # from CC2650 datasheet
CC2650_RADIO_CPU_SLEEP_CURRENT      = 1.335 # empirical
CC2650_RADIO_CPU_DEEP_SLEEP_CURRENT = 0.010 # empirical

###########################################

# for testbed: mapping between the node ID (Contiki_NG) and device ID (testbed)
node_id_to_device_id = {}

###########################################

class NodeStats:
    def __init__(self, id):
        self.id = id

        # intermediate metrics
        self.is_valid = False
        self.has_joined = False
        self.tsch_join_time_sec = None
        self.rpl_join_time_msec = None
        self.rpl_time_joined_msec = 0
        self.rpl_parent = None
        self.rpl_parent_lladdr = None
        self.max_seqnum_sent = 0
        self.seqnums_received_on_root = [[],[]]
        self.seqnums_sent = [[],[]]
        self.e2e_delay = []
        self.parent_packets_tx = 0
        self.parent_packets_ack = 0
        self.parent_packets_queue_dropped = 0
        self.energest_cpu_on = 0
        self.energest_cpu_sleep = 0
        self.energest_cpu_deep_sleep = 0
        self.energest_radio_tx = 0
        self.energest_radio_rx = 0
        self.energest_radio_rx_joined = 0
        self.energest_total = 0
        self.energest_total_joined = 0
        self.energest_ticks_per_second = 1
        self.energest_joined = False
        self.energest_period_seconds = 60

        # final metrics (uninitialized)
        self.pdr = 0.0
        self.rpl_parent_changes = 0
        self.par = 0.0
        self.rdc = None
        self.rdc_joined = None
        self.charge = None
        self.avg_e2e_delay = 0
        self.jitter = 0

    # calculate the final metrics
    def calc(self):
        if self.energest_total:
            radio_on = self.energest_radio_tx + self.energest_radio_rx
            self.rdc = 100.0 * radio_on / self.energest_total

            cpu_on_sec = self.energest_cpu_on / self.energest_ticks_per_second
            cpu_sleep_sec = self.energest_cpu_sleep / self.energest_ticks_per_second
            cpu_deep_sleep_sec = self.energest_cpu_deep_sleep / self.energest_ticks_per_second
            radio_tx_sec = self.energest_radio_tx / self.energest_ticks_per_second
            radio_rx_sec = self.energest_radio_rx / self.energest_ticks_per_second

            self.charge = CC2650_RADIO_TX_CURRENT_MA * radio_tx_sec \
                + CC2650_RADIO_RX_CURRENT_MA * radio_rx_sec \
                + CC2650_RADIO_CPU_ON_CURRENT * cpu_on_sec \
                + CC2650_RADIO_CPU_SLEEP_CURRENT * cpu_sleep_sec \
                + CC2650_RADIO_CPU_DEEP_SLEEP_CURRENT * cpu_deep_sleep_sec

        else:
            print("warning: no energest results for {}".format(self.id))
            self.rdc = 0.0
            self.charge = 0.0

        if self.energest_total_joined:
            radio_on_joined = self.energest_radio_tx + self.energest_radio_rx_joined
            self.rdc_joined = 100.0 * radio_on_joined / self.energest_total_joined
        else:
            self.rdc_joined = 0


        if self.tsch_join_time_sec is None:
            print("node {} never associated TSCH".format(self.id))
            #return 0, 0, 0, 0, 0

        if self.rpl_time_joined_msec == 0:
            print("node {} never joined RPL DAG".format(self.id))
            return 0, 0, 0, 0, 0


        if self.max_seqnum_sent == 0:
            print("node {} never sent any data packets".format(self.id))
            return 0, 0, 0, 0, 0

        self.is_valid = True

        if self.parent_packets_tx:
            self.par = 100.0 * self.parent_packets_ack / self.parent_packets_tx
        else:
            self.par = 0.0

        expected = self.max_seqnum_sent
        actual = len(self.seqnums_received_on_root[0])
        if expected:
            self.pdr = 100.0 * actual / expected
        else:
            self.pdr = 0.0

        return self.parent_packets_tx, \
            self.parent_packets_ack, \
            self.parent_packets_queue_dropped, \
            self.max_seqnum_sent, \
            len(self.seqnums_received_on_root[0])


###########################################

def extract_macaddr(s):
    if "NULL" in s:
        return None
    return s

def extract_ipaddr(s):
    if "NULL" in s:
        return None
    return s

# (NULL IP addr) -> fe80::244:44:44:44
def extract_ipaddr_pair(fields):
    s = " ".join(fields)
    fields = s.split(" -> ")
    return extract_ipaddr(fields[0]), extract_ipaddr(fields[1])

def addr_to_id(addr):
    return int(addr.split(":")[-1], 16)

###########################################
# Parse a log file

def analyze_results(filename, is_testbed):
    nodes = {}

    in_initialization = True

    start_ts_unix = None

    sim_time_ms = None

    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if is_testbed:
                fields1 = line.split(";")
                fields2 = fields1[2].split()
                fields = fields1[:2] + fields2
            else:
                fields = line.split()

            try:
                # in milliseconds
                if is_testbed:
                    ts_unix = float(fields[0])
                    if start_ts_unix is None:
                        start_ts_unix = ts_unix
                    ts_unix -= start_ts_unix
                    ts = int(float(ts_unix) * 1000)
                    node = int(fields[1][3:])
                else:
                    ts = int(fields[0]) // 1000 # convert to ms
                    node = int(fields[1])
            except:
                # failed to extract timestamp
                # Test ended at simulation time: 3600000000
                if "Test ended" in line:
                    sim_time_ms = int(fields[5]) / 1000
                continue

            if node not in nodes:
                nodes[node] = NodeStats(node)

            if "association done" in line:
                # has_assoc.add(node)
                #nodes[node].seqnums = set()
                if nodes[node].tsch_join_time_sec is None:
                    nodes[node].tsch_join_time_sec = ts / 1000
                nodes[node].has_joined = True
                continue

            if "leaving the network" in line:
                nodes[node].has_joined = False
                nodes[node].energest_joined = False
                continue

            # 536000 2 [INFO: TSCH Queue] update time source: (NULL LL addr) -> 0001.0001.0001.0001
            if "update time source" in line:
                nodes[node].rpl_parent_lladdr = extract_macaddr(line.split(" -> ")[1]) #tsch_time_source
                continue

            # 2799582 3 [INFO: CSMA      ] sending to 0000.0000.0000.0000, len 10, seqno 128, queue length 1, free packets %zu
            if "sending to" in line:
                continue

            # 2799582 3 [INFO: TSCH      ] send packet to 0000.0000.0000.0000 with seqno 128, queue 1, len 1
            if "send packet to" in line:
                continue

            # 30194781 3 [INFO: CSMA      ] received packet from c10c.0000.0000.0002, seqno 19, len 10
            if "received packet from" in line:
                continue

            # 30194781 3 [INFO: TSCH      ] received from c10c.0000.0000.0002 with seqno 19
            if "received from" in line:
                continue

            # 2497128 2 [INFO: RPL       ] rpl_set_preferred_parent fe80::201:1:1:1 used to be NULL
            if "rpl_set_preferred_parent" in line:
                nodes[node].rpl_parent_changes += 1
                nodes[node].rpl_parent = extract_ipaddr(fields[6])
                if nodes[node].rpl_parent is not None:
                    print("Node {} joined RPL DAG through parent".format(nodes[node].id), "{}".format(nodes[node].rpl_parent), "at {} seconds".format(ts / 1000))
                    if nodes[node].rpl_join_time_msec is None:
                        nodes[node].rpl_join_time_msec = ts
                    nodes[node].has_joined = True
                continue

            # 377018480 76 [INFO: RPL       ] parent switch: (NULL IP addr) -> fe80::244:44:44:44
            if " parent switch: " in line:
                nodes[node].rpl_parent_changes += 1
                nodes[node].rpl_parent = extract_ipaddr_pair(fields[7:])[1]
                if nodes[node].rpl_parent is not None:
                    print("Node {} joined RPL DAG through parent".format(nodes[node].id), "{}".format(nodes[node].rpl_parent), "at {} seconds".format(ts / 1000))
                    if nodes[node].rpl_join_time_msec is None:
                        nodes[node].rpl_join_time_msec = ts
                    nodes[node].has_joined = True
                continue

            # 123047424 1 [INFO: App       ] rpl callback: new parent lladdr -> 0001.0001.0001.0001
            if "new parent lladdr" in line:
                nodes[node].rpl_parent_lladdr = extract_macaddr(line.split(" -> ")[1])
                continue

            # 2497128 1 [INFO: App       ] rpl callback: node has left the network
            if "node has left the network" in line:
                print("Node {} has left the network".format(nodes[node].id), "at {} seconds".format(ts / 1000))
                nodes[node].has_joined = False
                nodes[node].rpl_time_joined_msec += (ts - nodes[node].rpl_join_time_msec)
                nodes[node].rpl_join_time_msec = None
                nodes[node].energest_joined = False
                continue

            # 120904000 4 [INFO: App       ] app generate packet seqnum=1
            if "app generate packet" in line:
                seqnum = int(fields[8].split("=")[1])
                if is_testbed:
                    node_id = int(fields[9].split("=")[1])
                    node_id_to_device_id[node_id] = node
                nodes[node].max_seqnum_sent = max(nodes[node].max_seqnum_sent, seqnum)
                ind_s = nodes[node].seqnums_sent[0].index(seqnum) if seqnum in nodes[node].seqnums_sent[0] else None
                if ind_s is not None:
                    nodes[node].seqnums_sent[1][ind_s] = ts
                else:
                    nodes[node].seqnums_sent[0].append(seqnum)
                    nodes[node].seqnums_sent[1].append(ts)
                continue

            # 123047424 1 [INFO: App       ] app receive packet seqnum=1 from=fd00::208:8:8:8
            if "app receive packet" in line:
                seqnum = int(fields[8].split("=")[1])
                fromaddr = fields[9].split("=")[1]
                from_node = addr_to_id(fromaddr)
                if is_testbed:
                    from_node = node_id_to_device_id.get(from_node, 0)
                if from_node not in nodes:
                    nodes[from_node] = NodeStats(from_node)
                ind_r = nodes[from_node].seqnums_received_on_root[0].index(seqnum) if seqnum in nodes[from_node].seqnums_received_on_root[0] else None
                if ind_r is not None:
                    nodes[from_node].seqnums_received_on_root[1][ind_r] = ts
                else:
                    nodes[from_node].seqnums_received_on_root[0].append(seqnum)
                    nodes[from_node].seqnums_received_on_root[1].append(ts)
                    ind_r = nodes[from_node].seqnums_received_on_root[0].index(seqnum)
                try:
                    ind_s = nodes[from_node].seqnums_sent[0].index(seqnum)
                    nodes[from_node].e2e_delay.append(nodes[from_node].seqnums_received_on_root[1][ind_r] - nodes[from_node].seqnums_sent[1][ind_s])
                except:
                    print("WARNING: Received seqnum not in sent index.")
                    continue
                continue

            # 600142000 28 [INFO: Link Stats] num packets: tx=0 ack=0 rx=0 queue_drops=0 to=0014.0014.0014.0014
            if "num packets" in line:
                tx = int(fields[7].split("=")[1])
                ack = int(fields[8].split("=")[1])
                rx = int(fields[9].split("=")[1])
                queue_drops = int(fields[10].split("=")[1])
                to_addr = fields[11].split("=")[1]
                # only account for the (current) parent node
                if nodes[node].rpl_parent_lladdr == to_addr:
                    nodes[node].parent_packets_tx += tx
                    nodes[node].parent_packets_ack += ack
                    nodes[node].parent_packets_queue_dropped += queue_drops
                continue

            # 960073000 8 [INFO: Energest  ] Total time  :   60000000
            # 960073000 8 [INFO: Energest  ] CPU         :   60000000/  60000000 (69 permil)
            # 960073000 8 [INFO: Energest  ] LPM         :          0/  60000000 (0 permil)
            # 960073000 8 [INFO: Energest  ] Deep LPM    :          0/  60000000 (0 permil)
            # 960073000 8 [INFO: Energest  ] Radio Tx    :      49216/  60000000 (0 permil)
            # 960073000 8 [INFO: Energest  ] Radio Rx    :    2470552/  60000000 (41 permil)
            # 960073000 8 [INFO: Energest  ] Radio total :    2519768/  60000000 (41 permil)
            if "INFO: Energest" in line:
                if "Period" in line:
                    nodes[node].energest_period_seconds = int(fields[9][1:])
                elif "Total time" in line:
                    total = int(fields[8])
                    nodes[node].energest_total += total
                    nodes[node].energest_ticks_per_second = total / nodes[node].energest_period_seconds
                    if nodes[node].energest_joined:
                        nodes[node].energest_total_joined += total
                else:
                    ticks = int(fields[8][:-1])
                    if "CPU" in line:
                        nodes[node].energest_cpu_on += ticks
                    elif "Deep LPM" in line:
                        nodes[node].energest_cpu_sleep += ticks
                    elif "LPM" in line:
                        nodes[node].energest_cpu_deep_sleep += ticks
                    elif "Radio Tx" in line:
                        nodes[node].energest_radio_tx += ticks
                    elif "Radio Rx" in line:
                        nodes[node].energest_radio_rx += ticks
                        if nodes[node].energest_joined:
                            nodes[node].energest_radio_rx_joined += ticks
                        # update the state
                        nodes[node].energest_joined = nodes[node].has_joined
                    continue

    if sim_time_ms is None:
        # failed to parse sim end time
        print("WARNING: Could not parse the total simulation time. Using last timestamp recorded instead.")
        sim_time_ms = ts
    r = []
    # link layer PAR
    total_ll_sent = 0
    total_ll_acked = 0
    total_ll_queue_dropped = 0
    # end to end PDR
    total_e2e_sent = 0
    total_e2e_received = 0
    for k in sorted(nodes.keys()):
        n = nodes[k]
        if n.id == COORDINATOR_ID:
            continue
        if n.rpl_join_time_msec is not None:
            n.rpl_time_joined_msec += (sim_time_ms - n.rpl_join_time_msec)
        ll_sent, ll_acked, ll_queue_dropped, e2e_sent, e2e_received = n.calc()
        n.avg_e2e_delay = np.mean(n.e2e_delay)
        n.jitter = n.e2e_delay[0]/16
        i = 1
        while i < len(n.e2e_delay):
            n.jitter = n.jitter*(15/16) + abs(n.e2e_delay[i] - n.e2e_delay[i-1])/16
            i += 1
        if n.is_valid or PLOT_ALL_NODES:
            d = {
                "id": n.id,
                "pdr": n.pdr,
                "par": n.par,
                "packets_sent": n.max_seqnum_sent,
                "rpl_switches": n.rpl_parent_changes,
                "duty_cycle": n.rdc,
                "duty_cycle_joined": n.rdc_joined,
                "charge": n.charge,
                "time_joined": n.rpl_time_joined_msec / 1000,
                "avg_e2e_delay": n.avg_e2e_delay,
                "jitter": n.jitter
            }
            r.append(d)
            total_ll_sent += ll_sent
            total_ll_acked += ll_acked
            total_ll_queue_dropped += ll_queue_dropped
            total_e2e_sent += e2e_sent
            total_e2e_received += e2e_received
    ll_par = 100.0 * total_ll_acked / total_ll_sent if total_ll_sent else 0.0
    e2e_pdr = 100.0 * total_e2e_received / total_e2e_sent if total_e2e_sent else 0.0
    return r, ll_par, total_ll_queue_dropped, e2e_pdr

#######################################################
# Plot the results of a given metric as a bar chart

def plot(results, metric, ylabel):
    pl.figure(figsize=(5, 4))

    data = [r[metric] for r in results]
    x = range(len(data))
    barlist = pl.bar(x, data, width=0.4)

    for b in barlist:
        b.set_color("orange")
        b.set_edgecolor("black")
        b.set_linewidth(1)

    ids = [r["id"] for r in results]
    pl.xticks(x, [str(u) for u in ids], rotation=90)
    pl.xlabel("Node ID")
    pl.ylabel(ylabel)

    if metric == "pdr":
        miny = min(80, min(data))
        pl.ylim([miny, 100])
    else:
        pl.ylim(ymin=0)

    pl.savefig("plot_{}.pdf".format(metric), format="pdf", bbox_inches='tight')
    pl.close()

#######################################################
# Run the application

def main():
    input_file = LOG_FILE
    if len(sys.argv) > 1:
        # change from the default
        input_file = sys.argv[1]

    if not os.access(input_file, os.R_OK):
        print('The input file "{}" does not exist'.format(input_file))
        exit(-1)

    with open(input_file, "r") as f:
        is_testbed = "Starting COOJA logger" not in f.read()

    results, ll_par, ll_queue_dropped, e2e_pdr = analyze_results(input_file, is_testbed)

    print("Link-layer PAR={:.2f} ({} packets queue dropped) End-to-end PDR={:.2f}".format(
        ll_par, ll_queue_dropped, e2e_pdr))

    plot(results, "pdr", "Packet Delivery Ratio, %")
    plot(results, "par", "Packet Acknowledgement Ratio, %")
    plot(results, "packets_sent", "Number of packets sent")
    plot(results, "rpl_switches", "RPL parent switches")
    plot(results, "duty_cycle", "Radio Duty Cycle, %")
    plot(results, "duty_cycle_joined", "Joined Radio Duty Cycle, %")
    plot(results, "charge", "Charge consumption, mC")
    plot(results, "time_joined", "Total time joined to DAG, s")
    plot(results, "avg_e2e_delay", "Average E2E delay, ms")
    plot(results, "jitter", "E2E jitter, ms")

#######################################################

if __name__ == '__main__':
    main()
