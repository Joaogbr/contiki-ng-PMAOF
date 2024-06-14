#!/usr/bin/env python3

import os
import sys
import argparse
import numpy as np
import matplotlib.pyplot as pl
import matplotlib.patches as mpatches
from scipy import stats

###########################################

SELF_PATH = os.path.dirname(os.path.abspath(__file__))
SIM_PATH = SELF_PATH
LOG_FILE = 'analysis_results.txt'

###########################################

output_stream = []

###########################################
# Parse a log file

def analyze_results(filename):
    with open(filename, "r") as f:
        lines = f.readlines()

        #Link-layer PAR = 34.01 (total of 0 packets dropped in queue) End-to-end PDR = [ mean= 64.84 moe= 15.76 ]
        fields = lines[-5].split()
        ll_par, total_ll_queue_dropped, e2e_pdr, e2e_pdr_moe = float(fields[3]), int(fields[6]), float(fields[16]), float(fields[18])
        #Total packets sent = 22112 Total packets received = 6833
        fields = lines[-4].split()
        total_e2e_sent, total_e2e_received = int(fields[4]), int(fields[9])
        #No. of parent switches = [ mean= 1.58 moe= 0.30 ] Avg. time joined = 3580.32 s
        fields = lines[-3].split()
        avg_rpl_p_switches, avg_rpl_p_switches_moe, avg_rpl_time_joined = float(fields[7]), float(fields[9]), float(fields[15])
        #End-to-end total delay = [ mean= 81.04 moe= 8.05 ] ms End-to-end total jitter = [ mean= 28.44 moe= 3.30 ] ms
        fields = lines[-2].split()
        avg_total_e2e_delay, avg_total_e2e_delay_moe, avg_total_e2e_jitter, avg_total_e2e_jitter_moe = float(fields[6]), float(fields[8]), float(fields[17]), float(fields[19])
        #Jain's Justice Index = 0.925
        fields = lines[-1].split()
        justice_index = float(fields[4])

        d = {
            "no.": os.path.basename(os.path.dirname(filename)),
            "PDR": e2e_pdr,
            "pdr_moe": e2e_pdr_moe,
            "Trocas": avg_rpl_p_switches,
            "rpl_switches_moe": avg_rpl_p_switches_moe,
            "Atraso": avg_total_e2e_delay,
            "avg_e2e_delay_moe": avg_total_e2e_delay_moe,
            "Jitter": avg_total_e2e_jitter,
            "jitter_moe": avg_total_e2e_jitter_moe,
            "Justica": justice_index
        }
    return d, ll_par, total_ll_queue_dropped, total_e2e_sent, total_e2e_received, avg_rpl_time_joined, justice_index

#######################################################
# Get the margin of the confidence interval

def confint(data, confidence):
    data = np.array(data)[~np.isnan(data)]
    mean = np.mean(data)
    sem = stats.sem(data)
    cum_prob = 1 - (1 - confidence) / 2
    margin_of_error = stats.t.ppf(cum_prob, len(data) - 1) * sem
    return margin_of_error

#######################################################
# Plot the results of a given metric as a bar chart

def plot(results, metric, ylabel, ranges_list):
    fig, ax = pl.subplots(1, figsize=(5, 4))

    data = [r[metric] for r in results]
    data_means = []
    data_moes = []
    x = range(len(ranges_list) - 1)
    for i in x:
        data_means.append(np.mean(data[ranges_list[i]:ranges_list[i+1] - 1]))
        data_moes.append(confint(data[ranges_list[i]:ranges_list[i+1] - 1], 0.95))
    barlist = pl.bar(x, data_means, yerr=data_moes, error_kw=dict(ecolor='darkslategray', lw=1, capsize=5, capthick=1), width=0.4)

    for b in barlist:
        b.set_color("orange") if (barlist.index(b) % 2 == 0) else b.set_color("darkblue")
        b.set_edgecolor("black")
        b.set_linewidth(1)

    ids = [""]
    #ticks = [pos + 0.5 + range(len(ids)).index(pos) for pos in range(len(ids))]
    pl.xticks(x, [str(u) for u in ids], fontsize=7)
    pl.yticks(fontsize=7)
    pl.title("Resultados do Cenário de Testes", fontsize=10)
    #pl.xlabel("Velocidade máxima", fontsize=8)
    pl.ylabel(ylabel, fontsize=8)
    MRHOF = mpatches.Patch(color='orange', label='RPL Original')
    MVMTOF = mpatches.Patch(color='darkblue', label='Proposta')
    pl.legend(handles=[MRHOF, MVMTOF], loc='upper right', fontsize=8)

    if metric == "PDR":
        pl.ylim([0, 100])
    elif metric == "Atraso":
        pl.ylim([0, 120])
    elif metric == "Jitter":
        pl.ylim([0, 60])
    elif metric == "Trocas":
        pl.ylim([0, 8])
    elif metric == "Justica":
        pl.ylim([0, 1.2])
    else:
        pl.ylim(ymin=0)

    pl.savefig(os.path.join(SIM_PATH, "{}.pdf".format(metric)), format="pdf", bbox_inches='tight')
    pl.close()

#######################################################
# Sort directories numerically

def num_sort(dir):
    return int(dir.split("/")[-1])

#######################################################
# Run the application

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--to-dir', default=None, dest='to_dir',
        help='Specify name of directory where the simulation dirs are placed')
    parser.add_argument('--fname', default=LOG_FILE, dest='fname',
        help='Specify name of results file')
    pargs = parser.parse_args()
    results = []

    if pargs.to_dir is not None:
        global SIM_PATH
        SIM_PATH = os.path.join(SELF_PATH, pargs.to_dir)
    subdirs = [x[0] for x in os.walk(SIM_PATH)][1:]
    subdirs = sorted(subdirs, key=num_sort)
    output_file = os.path.join(SIM_PATH, "multisim-analysis_results.txt")
    ll_par, ll_queue_dropped, e2e_sent, e2e_received, avg_rpl_tj, avg_justice_index = (np.zeros(len(subdirs)) for _ in range(6))

    for subdir in subdirs:
        input_file = os.path.join(subdir, pargs.fname)
        if not os.access(input_file, os.R_OK):
            print('The input file "{}" does not exist'.format(input_file))
            continue
        idx = subdirs.index(subdir)
        d, ll_par[idx], ll_queue_dropped[idx], e2e_sent[idx], e2e_received[idx], avg_rpl_tj[idx], avg_justice_index[idx] = analyze_results(input_file)
        results.append(d)

    ranges = [0, 10]
    plot(results, "PDR", "PDR, em porcentagem", ranges)
    plot(results, "Trocas", "Quantidade de trocas de pai", ranges)
    plot(results, "Atraso", "Atraso médio fim-a-fim, em ms", ranges)
    plot(results, "Jitter", "Jitter médio fim-a-fim, em ms", ranges)
    plot(results, "Justica", "Índice de justiça do atraso", ranges)

    if os.path.isfile(output_file):
        os.remove(output_file)
    of = open(output_file, "w")

    e2e_pdr = [r["PDR"] for r in results]
    rpl_ps = [r["Trocas"] for r in results]
    avg_e2e_delay = [r["Atraso"] for r in results]
    avg_e2e_jitter = [r["Jitter"] for r in results]

    output_stream.append("Link-layer PAR: [mean = {:.2f}/moe = {:.2f}] (total packets dropped in queue: [mean = {:.2f}/moe = {:.2f}]) End-to-end PDR: [mean = {:.2f}/moe = {:.2f}]".format(
        np.mean(ll_par), confint(ll_par, 0.95), np.mean(ll_queue_dropped), confint(ll_queue_dropped, 0.95), np.mean(e2e_pdr), confint(e2e_pdr, 0.95)))
    output_stream.append("Total packets sent: [mean = {:.2f}/moe = {:.2f}] Total packets received: [mean = {:.2f}/moe = {:.2f}]".format(
        np.mean(e2e_sent), confint(e2e_sent, 0.95), np.mean(e2e_received), confint(e2e_received, 0.95)))
    output_stream.append("Avg. no. of parent switches: [mean = {:.2f}/moe = {:.2f}] Avg. time joined: [mean = {:.2f} s/moe = {:.2f} s]".format(
        np.mean(rpl_ps), confint(rpl_ps, 0.95), np.mean(avg_rpl_tj), confint(avg_rpl_tj, 0.95)))
    output_stream.append("Avg. end-to-end total delay: [mean = {:.2f} ms/moe = {:.2f} ms] Avg. end-to-end total jitter: [mean = {:.2f} ms/moe = {:.2f} ms]".format(
        np.mean(avg_e2e_delay), confint(avg_e2e_delay, 0.95), np.mean(avg_e2e_jitter), confint(avg_e2e_jitter, 0.95)))
    output_stream.append("Avg. Justice Index: [mean = {:.3f}/moe = {:.3f}]".format(np.mean(avg_justice_index), confint(avg_justice_index, 0.95)))

    print(*output_stream, sep = "\n", file = sys.stdout)
    print(*output_stream, sep = "\n", file = of)

#######################################################

if __name__ == '__main__':
    main()
