#! /bin/bash
set -e # exit on error
# set -x # print commands

main() {
    # bench_small
    bench_large
}

bench_small() {
    for epoch in 1 2 3 4 5 6 7 8 9 10; do
        for B in 1 32 1024; do
            for N in 100 1000 10000 25000 50000; do
                echo "runing epoch $epoch with $B bytes and $N requests"
                /usr/bin/time -v -o out.txt sd-bus-client -n $N -b $B -t 0 -q

                # extract time from /usr/bin/time output
                time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
                echo "$epoch $B $N $time" >> results.txt
                
                sleep 1 # small delay between requests
            done
        done
    done
}

bench_large() {
    for epoch in 1 2 3; do
        for B in $(numfmt --from=iec 10M 20M 30M 40M 50M); do
            for N in 10 25 50 100; do
                echo "runing epoch $epoch with $B bytes and $N requests"
                /usr/bin/time -v -o out.txt sd-bus-client -n $N -b $B -t 0 -q

                # extract time from /usr/bin/time output
                time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
                echo "$epoch $B $N $time" >> results_large.txt
                
                sleep 1 # small delay between requests
            done
        done
    done
}

main

# clean up
rm out.txt