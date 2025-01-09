#!/bin/bash

trap "exit" SIGINT

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <directory> <start> <end>"
    exit 1
fi

dir=$1
start=$2
end=$3

for ((i=start; i<=end; i++))
do
    python3 run-cooja.py --to-dir "$dir/$i"
done
