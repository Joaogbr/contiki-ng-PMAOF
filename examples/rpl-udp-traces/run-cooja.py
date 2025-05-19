#!/usr/bin/env python3

import sys
import os
import argparse
from subprocess import Popen, PIPE, STDOUT, CalledProcessError

# get the path of this example
SELF_PATH = os.path.dirname(os.path.abspath(__file__))
SIM_PATH = SELF_PATH
# move two levels up
CONTIKI_PATH = os.path.dirname(os.path.dirname(SELF_PATH))

COOJA_PATH = os.path.normpath(os.path.join(CONTIKI_PATH, "tools", "cooja"))
cooja_input = 'cooja.csc'
cooja_output = 'COOJA.testlog'

#######################################################
# Create separate folder for simulation

def make_dir(mkdir):
    if mkdir is not None:
        global SIM_PATH
        SIM_PATH = os.path.join(SELF_PATH, mkdir)
        global cooja_output
        cooja_output = os.path.join(SIM_PATH, cooja_output)
        if not os.path.exists(SIM_PATH):
            os.makedirs(SIM_PATH)

#######################################################
# Run a child process and get its output

def run_subprocess(args, input_string):
    retcode = -1
    stdoutdata = ''
    try:
        proc = Popen(args, stdout=PIPE, stderr=STDOUT, stdin=PIPE, shell=True, universal_newlines=True)
        (stdoutdata, stderrdata) = proc.communicate(input_string)
        if not stdoutdata:
            stdoutdata = '\n'
        if stderrdata:
            stdoutdata += stderrdata + '\n'
        retcode = proc.returncode
    except OSError as e:
        sys.stderr.write("run_subprocess OSError:" + str(e))
    except CalledProcessError as e:
        sys.stderr.write("run_subprocess CalledProcessError:" + str(e))
        retcode = e.returncode
    except Exception as e:
        sys.stderr.write("run_subprocess exception:" + str(e))
    finally:
        return (retcode, stdoutdata)

#############################################################
# Run a single instance of Cooja on a given simulation script

def execute_test(cooja_file):
    # cleanup
    try:
        os.remove(cooja_output)
    except FileNotFoundError as ex:
        pass
    except PermissionError as ex:
        print("Cannot remove previous Cooja output:", ex)
        return False

    filename = os.path.join(SELF_PATH, cooja_file)
    args = " ".join([COOJA_PATH + "/gradlew --no-watch-fs --parallel --build-cache -p", COOJA_PATH, "run --args='--contiki=" + CONTIKI_PATH, "--no-gui", "--logdir=" + SIM_PATH, filename + "'"])
    sys.stdout.write("  Running Cooja, args={}\n".format(args))

    (retcode, output) = run_subprocess(args, '')
    if retcode != 0:
        sys.stderr.write("Failed, retcode=" + str(retcode) + ", output:")
        sys.stderr.write(output)
        return False

    sys.stdout.write("  Checking for output...")

    is_done = False
    with open(cooja_output, "r") as f:
        for line in f.readlines():
            line = line.strip()
            if line == "TEST OK":
                sys.stdout.write(" done.\n")
                is_done = True
                continue

    if not is_done:
        sys.stdout.write("  test failed.\n")
        return False

    sys.stdout.write(" test done\n")
    return True

#######################################################
# Run the application

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--to-dir', default=None, dest='to_dir',
        help='Specify name of new directory where the simulation files will be placed')
    parser.add_argument('--fname', default=cooja_input, dest='fname',
        help='Specify name of simulation file')
    pargs = parser.parse_args()

    make_dir(pargs.to_dir)
    input_file = pargs.fname

    if not os.access(input_file, os.R_OK):
        print('Simulation script "{}" does not exist'.format(input_file))
        exit(-1)

    print('Using simulation script "{}"'.format(input_file))
    if not execute_test(input_file):
        exit(-1)

#######################################################

if __name__ == '__main__':
    main()
