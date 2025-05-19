#!/bin/bash

trap "exit" SIGINT

# Check if the correct number of arguments is provided
if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <fname> <directory> <start> <end>"
    exit 1
fi

fname=$1
dir=$2
start=$3
end=$4

for ((i=start; i<=end; i++))
do
    python3 run-cooja.py --fname "$fname" --to-dir "$dir/$i"
done
