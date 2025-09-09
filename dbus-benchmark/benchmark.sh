#! /bin/bash
set -e # exit on error
# set -x # print commands

# /usr/bin/time -v ./bin/sd-bus-client -n 100000 -b 32 -t 0 -q
# for N in 10000 25000 50000 75000 100000; do
for epoch in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    for B in 1 32 1024; do
        for N in 100 1000 10000 25000 50000; do
            /usr/bin/time -v -o out.txt sd-bus-client -n $N -b $B -t 0 -q
            time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
            echo "$B $N $time" >> results.txt
            
            # Add a small delay between requests
            sleep 1
        done
    done
done