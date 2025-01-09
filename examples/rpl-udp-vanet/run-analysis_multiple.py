#!/usr/bin/env python3

import os
import sys
import argparse
import numpy as np
import matplotlib.pyplot as pl
from scipy import stats
import re
from collections import OrderedDict

###########################################

# If set to true, all nodes are plotted, even those with no valid data
PLOT_ALL_NODES = True

###########################################

SELF_PATH = os.path.dirname(os.path.abspath(__file__))
SIM_PATH = SELF_PATH
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

output_stream = []

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
        self.avg_e2e_delay = np.nan
        self.jitter = np.nan

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
            output_stream.append("warning: no energest results for {}".format(self.id))
            self.rdc = 0.0
            self.charge = 0.0

        if self.energest_total_joined:
            radio_on_joined = self.energest_radio_tx + self.energest_radio_rx_joined
            self.rdc_joined = 100.0 * radio_on_joined / self.energest_total_joined
        else:
            self.rdc_joined = 0


        #if self.tsch_join_time_sec is None:
            #output_stream.append("node {} never associated TSCH".format(self.id))
            #return 0, 0, 0, 0, 0

        # if self.rpl_time_joined_msec == 0:
            # output_stream.append("node {} never joined RPL DAG".format(self.id))
            # output_stream.append("node {} actual {} expected {}".format(self.id, self.seqnums_received_on_root[0], self.seqnums_sent[0]))
            # return 0, 0, 0, 0, 0


        if not len(self.seqnums_sent[0]):
            # output_stream.append("node {} never sent any data packets".format(self.id))
            # output_stream.append("node {} actual {} expected {}".format(self.id, self.seqnums_received_on_root[0], self.seqnums_sent[0]))
            return 0, 0, 0, 0, 0

        self.is_valid = True

        if self.parent_packets_tx:
            self.par = 100.0 * self.parent_packets_ack / self.parent_packets_tx
        else:
            self.par = 0.0

        expected = len(self.seqnums_sent[0])
        actual = len(self.seqnums_received_on_root[0])
        # output_stream.append("node {} actual {} expected {}".format(self.id, actual, expected))
        if expected:
            self.pdr = 100.0 * actual / expected
        else:
            self.pdr = 0.0

        if len(self.e2e_delay):
            self.avg_e2e_delay = np.mean(self.e2e_delay)
            if len(self.e2e_delay) >= 2:
                self.jitter = self.e2e_delay[1] - self.e2e_delay[0]
                i = 2
                while i < len(self.e2e_delay):
                    self.jitter = self.jitter*(15/16) + abs(self.e2e_delay[i] - self.e2e_delay[i-1])/16
                    i += 1

        return self.parent_packets_tx, \
            self.parent_packets_ack, \
            self.parent_packets_queue_dropped, \
            len(self.seqnums_sent[0]), \
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

            # 4358536 15 [INFO: RPL       ] rpl_set_preferred_parent: used to be NULL, now is fe80::c30c:0:0:1
            if "rpl_set_preferred_parent" in line:
                nodes[node].rpl_parent_changes += 1
                nodes[node].rpl_parent = extract_ipaddr(fields[12])
                if nodes[node].rpl_parent is not None:
                    # output_stream.append("Node {} joined RPL DAG through parent".format(nodes[node].id) + " {}".format(nodes[node].rpl_parent) + " at {} seconds".format(ts / 1000))
                    if nodes[node].rpl_join_time_msec is None:
                        nodes[node].rpl_join_time_msec = ts
                    nodes[node].has_joined = True
                # else:
                    # output_stream.append("Node {} switched to NULL parent".format(nodes[node].id) + " at {} seconds".format(ts / 1000))
                continue

            # 377018480 76 [INFO: RPL       ] parent switch: (NULL IP addr) -> fe80::244:44:44:44
            if " parent switch: " in line:
                nodes[node].rpl_parent_changes += 1
                nodes[node].rpl_parent = extract_ipaddr_pair(fields[7:])[1]
                if nodes[node].rpl_parent is not None:
                    # output_stream.append("Node {} joined RPL DAG through parent".format(nodes[node].id) + " {}".format(nodes[node].rpl_parent) + " at {} seconds".format(ts / 1000))
                    if nodes[node].rpl_join_time_msec is None:
                        nodes[node].rpl_join_time_msec = ts
                    nodes[node].has_joined = True
                # else:
                    # output_stream.append("Node {} switched to NULL parent".format(nodes[node].id) + " at {} seconds".format(ts / 1000))
                continue

            # 4363361 15 [INFO: RPL       ] new parent lladdr -> c10c.0000.0000.0001
            if "new parent lladdr" in line:
                nodes[node].rpl_parent_lladdr = extract_macaddr(line.split(" -> ")[1])
                continue

            # 4753976 19 [INFO: RPL       ] node has left the network
            if "node has left the network" in line:
                output_stream.append("Node {} has left the network".format(nodes[node].id) + " at {} seconds".format(ts / 1000))
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
                if seqnum in nodes[node].seqnums_sent[0]:
                    # Could be a duplicate!
                    output_stream.append("WARNING: Application message with seqnum = {}".format(seqnum) + " already sent. Might be duplicate")
                    #if seqnum not in nodes[node].seqnums_received_on_root[0]: # This is not failproof, maybe never update ts at all?
                        #ind_s = nodes[node].seqnums_sent[0].index(seqnum)
                        #nodes[node].seqnums_sent[1][ind_s] = ts
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
                if seqnum in nodes[from_node].seqnums_received_on_root[0]:
                    # Duplicates should be dropped by link layer, so this condition should never be met
                    output_stream.append("WARNING: Application message with seqnum = {}".format(seqnum) + " already received. Must be duplicate")
                    #ind_r = nodes[from_node].seqnums_received_on_root[0].index(seqnum)
                    #nodes[from_node].seqnums_received_on_root[1][ind_r] = ts
                else:
                    nodes[from_node].seqnums_received_on_root[0].append(seqnum)
                    nodes[from_node].seqnums_received_on_root[1].append(ts)
                    ind_r = len(nodes[from_node].seqnums_received_on_root[0]) - 1
                    try:
                        ind_s = nodes[from_node].seqnums_sent[0].index(seqnum)
                        nodes[from_node].e2e_delay.append(nodes[from_node].seqnums_received_on_root[1][ind_r] - nodes[from_node].seqnums_sent[1][ind_s])
                    except:
                        output_stream.append("WARNING: Received seqnum = {}".format(seqnum) + " not in sent index")
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
        output_stream.append("WARNING: Could not parse the total simulation time. Using last timestamp recorded instead.")
        sim_time_ms = ts
    r = []
    # link layer PAR
    total_ll_sent = 0
    total_ll_acked = 0
    total_ll_queue_dropped = 0
    # RPL metrics
    total_rpl_parent_switches = 0
    total_rpl_time_joined = 0
    # end to end PDR
    total_e2e_sent = 0
    total_e2e_received = 0
    # end to end delay metrics
    total_e2e_delay = 0
    total_e2e_jitter = 0
    # justice index
    ji_numerator = 0
    ji_denominator = 0
    justice_index = 0

    discnt = 0
    for k in sorted(nodes.keys()):
        n = nodes[k]
        if n.id == COORDINATOR_ID:
            continue
        if n.rpl_join_time_msec is not None:
            n.rpl_time_joined_msec += (sim_time_ms - n.rpl_join_time_msec)
        ll_sent, ll_acked, ll_queue_dropped, e2e_sent, e2e_received = n.calc()
        if n.is_valid or PLOT_ALL_NODES:
            d = {
                "id": n.id,
                "pdr": n.pdr,
                "par": n.par,
                "packets_sent": len(n.seqnums_sent[0]),
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
            total_rpl_parent_switches += n.rpl_parent_changes
            total_rpl_time_joined += n.rpl_time_joined_msec / 1000
            total_e2e_sent += e2e_sent
            total_e2e_received += e2e_received
            # Do not count if no messages got received at root
            if len(n.seqnums_received_on_root[0]):
                ji_numerator += n.avg_e2e_delay
                ji_denominator += n.avg_e2e_delay**2
            else:
                discnt += 1
    ll_par = 100.0 * total_ll_acked / total_ll_sent if total_ll_sent else 0.0
    avg_rpl_time_joined = total_rpl_time_joined / (k-1)
    justice_index = (ji_numerator ** 2) / (ji_denominator * (k-1)) if (k-discnt-1) else 0.0
    return r, ll_par, total_ll_queue_dropped, total_e2e_sent, total_e2e_received, avg_rpl_time_joined, justice_index

#######################################################
# Plot the results of a given metric as a bar chart

def plot(results, metric, ylabel, file_name):
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

    pl.ylim(ymin=0)

    pl.savefig(os.path.join(SIM_PATH, "{}_{}.pdf".format(file_name, metric)), format="pdf", bbox_inches='tight')
    pl.close()


def get_files(caminho_da_pasta):
    try:
        itens_na_pasta = os.listdir(caminho_da_pasta)
        arquivos_na_pasta = [item for item in itens_na_pasta if os.path.isfile(os.path.join(caminho_da_pasta, item))]
        return arquivos_na_pasta
    except FileNotFoundError:
        print(f"A pasta '{caminho_da_pasta}' não existe.")
        return []
    except PermissionError:
        print(f"Não há permissão para acessar a pasta '{caminho_da_pasta}'.")
        return []
    except Exception as e:
        print(f"Ocorreu um erro ao tentar listar os arquivos: {str(e)}")
        return []


# Expressão regular para corresponder ao número

# Função para extrair o número
def get_file_number(nome_arquivo):
    padrao_numero = r'_([+-]?\d+)'
    correspondencia = re.search(padrao_numero, nome_arquivo)
    return int(correspondencia.group(1)) if correspondencia else None

def divide_dict_values(dictionary, divisor, multiplicador_pct, divisor_pct):
    """
    Divide todos os valores de um dicionário por um divisor.
    
    Parâmetros:
    dictionary (dict): O dicionário a ser dividido.
    divisor (float): O número pelo qual dividir cada valor.
    
    Retorna:
    dict: Um novo dicionário com os valores divididos.
    """
    return {k: (v / divisor)*multiplicador_pct/divisor_pct for k, v in dictionary.items()}

def get_ordered_file_numbers(dict_list):
    valid_keys = []
    for d in dict_list:
        valid_keys.extend(k for k in d.keys() if k is not None)
    return sorted(set(valid_keys)) 

def get_notified_cars(input_file):
    unique_ids = set()
    
    try:
        with open(input_file, 'r') as file:
            for line in file:
                match = re.search(r'(\d+)\s+\[INFO: App\s+\]\s+Received request', line)
                if match:
                    id_str = match.group(1)
                    if id_str.isdigit():  # Verifica se o ID é um número inteiro
                        unique_ids.add(int(id_str))
    
    except FileNotFoundError:
        print(f"Arquivo {file_path} não encontrado.")
        return 0
    except Exception as e:
        print(f"Erro ao ler o arquivo: {str(e)}")
        return 0
    
    print("Unique Ids {}\n".format(len(unique_ids)))
    return len(unique_ids)
#######################################################
# Run the application

def main(log_file):
    parser = argparse.ArgumentParser()
    parser.add_argument('--to-dir', default=None, dest='to_dir',
        help='Specify name of directory where the simulation result files will be placed')
    parser.add_argument('--fname', default=LOG_FILE, dest='fname',
        help='Specify name of log file')
    pargs = parser.parse_args()

    if pargs.to_dir is not None:
        global SIM_PATH
        SIM_PATH = os.path.join(SELF_PATH, pargs.to_dir)
    directory = os.path.join(SELF_PATH, scenario)
    files = get_files(directory)
    pdr_means = {}
    pdr_stds = {}
    delay_mean = {}
    jitter_mean = {}
    parent_changes_mean = {}
    notified_cars = {}
    for file in files:
        file_number = get_file_number(file)
        if file_number not in pdr_means:
            pdr_means[file_number] = 0
            pdr_stds[file_number] = 0
            delay_mean[file_number] = 0
            jitter_mean[file_number] = 0
            parent_changes_mean[file_number] = 0
            notified_cars[file_number] = 0
        print("file number {}\n".format(file_number))
        if "testlog" not in file:
            continue
        input_file = os.path.join(directory, file)
        output_file = os.path.join(SIM_PATH, "analysis_results.txt")
        if os.path.isfile(output_file):
            os.remove(output_file)
        of = open(output_file, "w")

        print("arquivo sendo analisado {}\n".format(input_file))
        if not os.access(input_file, os.R_OK):
            output_stream.append('The input file "{}" does not exist'.format(input_file))
            exit(-1)

        with open(input_file, "r") as f:
            is_testbed = "Starting COOJA logger" not in f.read()

        results, ll_par, ll_queue_dropped, e2e_sent, e2e_received, avg_rpl_tj, jus_idx = analyze_results(
            input_file, is_testbed)

        e2e_pdr = [r["pdr"] for r in results]
        rpl_ps = [r["rpl_switches"] for r in results]
        avg_e2e_delay = [r["avg_e2e_delay"] for r in results]
        avg_e2e_jitter = [r["jitter"] for r in results]


        output_stream.append("Link-layer PAR = {:.2f} (total of {} packets dropped in queue) End-to-end PDR = [ mean= {:.2f} std= {:.2f} ]".format(
            ll_par, ll_queue_dropped, np.mean(e2e_pdr), np.nanstd(e2e_pdr)))
        output_stream.append("Total packets sent = {} Total packets received = {}".format(
            e2e_sent, e2e_received))
        output_stream.append("No. of parent switches = [ mean= {:.2f} std= {:.2f} ] Avg. time joined = {:.2f} s".format(
            np.mean(rpl_ps), np.nanstd(rpl_ps), avg_rpl_tj))
        output_stream.append("End-to-end total delay = [ mean= {:.2f} std= {:.2f} ] ms End-to-end total jitter = [ mean= {:.2f} std= {:.2f} ] ms".format(
            np.nanmean(avg_e2e_delay), np.nanstd(avg_e2e_delay), np.nanmean(avg_e2e_jitter), np.nanstd(avg_e2e_jitter)))
        output_stream.append("Jain's Justice Index = {:.3f}".format(jus_idx))

        print(*output_stream, sep = "\n", file = sys.stdout)
        print(*output_stream, sep = "\n", file = of)

        # plot(results, "pdr", "Packet Delivery Ratio, %", file )
        # plot(results, "par", "Packet Acknowledgement Ratio, %")
        # plot(results, "packets_sent", "Number of packets sent")
        # plot(results, "rpl_switches", "RPL parent switches")
        # plot(results, "duty_cycle", "Radio Duty Cycle, %")
        # plot(results, "duty_cycle_joined", "Joined Radio Duty Cycle, %")
        # plot(results, "charge", "Charge consumption, mC")
        # plot(results, "time_joined", "Total time joined to DAG, s")
        # plot(results, "avg_e2e_delay", "Average E2E delay, ms", file)
        # plot(results, "jitter", "E2E jitter, ms")

        pdr_means[file_number] = pdr_means[file_number] + np.mean(e2e_pdr)
        pdr_stds[file_number] = pdr_stds[file_number] + np.nanstd(e2e_pdr)
        delay_mean[file_number] = delay_mean[file_number] + np.nanmean(avg_e2e_delay)
        jitter_mean[file_number] = jitter_mean[file_number] + np.nanmean(avg_e2e_jitter)
        parent_changes_mean[file_number] = parent_changes_mean[file_number] + np.nanmean(rpl_ps)
        notified_cars[file_number] = notified_cars[file_number] + get_notified_cars(input_file)

    pdr_means = divide_dict_values(pdr_means, 5, 100, 1)
    pdr_stds = divide_dict_values(pdr_stds, 5, 1, 1)
    jitter_mean = divide_dict_values(jitter_mean, 5, 100, 100)
    parent_changes_mean = divide_dict_values(parent_changes_mean, 5, 1, 1)
    delay_mean = divide_dict_values(delay_mean, 5, 100, 100)
    notified_cars = divide_dict_values(notified_cars, 5, 100, 85)
    # Ordenar os dicionários

    dict_list = [pdr_means, pdr_stds, delay_mean, notified_cars, jitter_mean, parent_changes_mean]
    ordered_file_numbers = get_ordered_file_numbers(dict_list)

    print("Ordered file numbers:", ordered_file_numbers)
    # ordered_file_numbers = sorted(set(pdr_means.keys()).union(pdr_stds.keys(), delay_mean.keys(), notified_cars.keys()))

    # Criar listas ordenadas para cada dicionário
    data_lists = [
        [dict_.get(file_number, np.nan) for file_number in ordered_file_numbers]
        for dict_ in dict_list
    ]

    fig, axs = pl.subplots(len(data_lists), figsize=(12, 3 * len(data_lists)))

    titles = ['Mean PDR', 'Standard Deviation of PDR', 'Mean End-to-End Delay', 'Notified Cars', 'Mean Jitter', 'Mean Parent switches']
    y_labels = ['PDR (%)', 'PDR (%)', 'Delay (ms)', 'Notified cars (%)', 'Jitter (ms)', 'Parent switches']

    for ax, title, y_label, data in zip(axs, titles, y_labels, data_lists):
        ax.plot(ordered_file_numbers, data, marker='o')
        ax.set_title(title)
        ax.set_xlabel(log_file)
        ax.set_ylabel(y_label)
        
        for x, y in zip(ordered_file_numbers, data):
            ax.annotate(f'{y:.2f}', (x, y), textcoords="offset points", xytext=(0,10), ha='center')

    pl.tight_layout()
    pl.savefig('{}/{}.jpg'.format(log_file, log_file), dpi=300, bbox_inches='tight')
    pl.close(fig)

#######################################################

scenarios = {
  "CF_ALPHA_CT",
  "CF_BETA_CT",
  "PATH_COST_RED_CT",
  "MF_LL_RED_CT",
  "MF_UL_RED_CT"
}

if __name__ == '__main__':
    # for scenario in scenarios:
    scenario = "CF_ALPHA_CT" 
    main(scenario)
    # main("CF_ALPHA_CT")