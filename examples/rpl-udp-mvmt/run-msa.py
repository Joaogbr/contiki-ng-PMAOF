#!/usr/bin/env python3

import os
import sys
import argparse
import time
import numpy as np
import matplotlib.pyplot as pl

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

        #Link-layer PAR = 21.80 (total of 0 packets dropped in queue) End-to-end PDR = 30.90
        fields = lines[-5].split()
        ll_par, total_ll_queue_dropped, e2e_pdr = fields[3], fields[6], fields[14]
        #Total packets sent = 22112 Total packets received = 6833
        fields = lines[-4].split()
        total_e2e_sent, total_e2e_received = fields[4], fields[9]
        #Avg. no. of parent switches = 6.02 Avg. time joined = 3505.07 s
        fields = lines[-3].split()
        avg_rpl_p_switches, avg_rpl_time_joined = fields[6], fields[11]
        #Avg. end-to-end total delay = 372.70 ms Avg. end-to-end total jitter = 161.72 ms
        fields = lines[-2].split()
        avg_total_e2e_delay, avg_total_e2e_jitter = fields[5], fields[12]
        #Jain's Justice Index = 0.925
        fields = lines[-1].split()
        justice_index = fields[4]
    return ll_par, total_ll_queue_dropped, total_e2e_sent, total_e2e_received, avg_rpl_p_switches, avg_rpl_time_joined, e2e_pdr, avg_total_e2e_delay, avg_total_e2e_jitter, justice_index

#######################################################
# Run the application

def main():
    no_sims = 10
    parser = argparse.ArgumentParser()
    parser.add_argument('--to-dir', default=None, dest='to_dir',
        help='Specify name of directory where the simulation dirs are placed')
    parser.add_argument('--fname', default=LOG_FILE, dest='fname',
        help='Specify name of results file')
    pargs = parser.parse_args()

    if pargs.to_dir is not None:
        global SIM_PATH
        SIM_PATH = os.path.join(SELF_PATH, pargs.to_dir)
    output_file = os.path.join(SIM_PATH, "multisim-analysis_results.txt")
    ll_par, ll_queue_dropped, e2e_sent, e2e_received, avg_rpl_ps, avg_rpl_tj, e2e_pdr, e2e_avgt_delay, e2e_avgt_jitter, avg_justice_index = (np.zeros(no_sims) for _ in range(10))

    for i in range(no_sims):
        input_file = os.path.join(os.path.join(SIM_PATH, str(i)), pargs.fname)
        if not os.access(input_file, os.R_OK):
            print('The input file "{}" does not exist'.format(input_file))
            continue
        ll_par[i], ll_queue_dropped[i], e2e_sent[i], e2e_received[i], avg_rpl_ps[i], avg_rpl_tj[i], e2e_pdr[i], e2e_avgt_delay[i], e2e_avgt_jitter[i], avg_justice_index[i] = analyze_results(input_file)

    if os.path.isfile(output_file):
        os.remove(output_file)
    of = open(output_file, "w")

    output_stream.append("Link-layer PAR: [mean = {:.2f}/std = {:.2f}] (total packets dropped in queue: [mean = {:.2f}/std = {:.2f}]) End-to-end PDR: [mean = {:.2f}/std = {:.2f}]".format(
        np.mean(ll_par), np.std(ll_par), np.mean(ll_queue_dropped), np.std(ll_queue_dropped), np.mean(e2e_pdr), np.std(e2e_pdr)))
    output_stream.append("Total packets sent: [mean = {:.2f}/std = {:.2f}] Total packets received: [mean = {:.2f}/std = {:.2f}]".format(
        np.mean(e2e_sent), np.std(e2e_sent), np.mean(e2e_received), np.std(e2e_received)))
    output_stream.append("Avg. no. of parent switches: [mean = {:.2f}/std = {:.2f}] Avg. time joined: [mean = {:.2f} s/std = {:.2f} s]".format(
        np.mean(avg_rpl_ps), np.std(avg_rpl_ps), np.mean(avg_rpl_tj), np.std(avg_rpl_tj)))
    output_stream.append("Avg. end-to-end total delay: [mean = {:.2f} ms/std = {:.2f} ms] Avg. end-to-end total jitter: [mean = {:.2f} ms/std = {:.2f} ms]".format(
        np.mean(e2e_avgt_delay), np.std(e2e_avgt_delay), np.mean(e2e_avgt_jitter), np.std(e2e_avgt_jitter)))
    output_stream.append("Avg. Justice Index: [mean = {:.3f}/std = {:.3f}]".format(np.mean(avg_justice_index), np.std(avg_justice_index)))

    print(*output_stream, sep = "\n", file = sys.stdout)
    print(*output_stream, sep = "\n", file = of)

#######################################################

if __name__ == '__main__':
    main()
