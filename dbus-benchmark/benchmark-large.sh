#!/bin/bash
set -e # exit on error
# set -x # print commands

LARGE_SIZES=(1048576 10485760 52428800 104857600 209715200)
SIZE_LABELS=("1MB" "10MB" "50MB" "100MB" "200MB")

for epoch in 1 2 3; do
    for i in "${!LARGE_SIZES[@]}"; do
        B=${LARGE_SIZES[$i]}
        LABEL=${SIZE_LABELS[$i]}
        echo "Running epoch $epoch: 1 request, $LABEL ($B bytes)"
        
        # Single request for large sizes due to memory and time constraints
        /usr/bin/time -v -o out.txt sd-bus-client -n 1 -b $B -t 0 -q
        time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
        echo "$epoch $B 1 $time" >> results_large.txt
        
        # Add a small delay between requests
        sleep 1
    done
done