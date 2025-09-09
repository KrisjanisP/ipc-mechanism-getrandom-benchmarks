#! /bin/bash
set -e # exit on error
# set -x # print commands

# kill any existing process listening on port 50051
lsof -ti:50051 | xargs -r kill -9

# cleanup function to kill server, remove out.txt on exit
cleanup() {
    echo "Cleaning up..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    # rm out.txt
}
trap cleanup EXIT

# start server
./build/randombytes_server &
SERVER_PID=$!

sleep 3 # wait for server to start

main() {
    # bench_small
    bench_large
}

bench_small() {
    # run small benchmark
    for epoch in 1 2 3 4 5 6 7 8 9 10; do
        for B in 1 32 1024; do
            for N in 100 1000 10000 25000; do
                echo "running epoch $epoch with $B bytes and $N requests"
                /usr/bin/time -v -o out.txt ./build/randombytes_client -n $N -b $B -t 0 -q
                time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
                echo "$epoch $B $N $time" >> results.txt
                
                sleep 1 # small delay between requests
            done
        done
    done
}

bench_large() {
    # run large benchmark
    for epoch in 1 2 3; do
        for B in $(numfmt --from=iec 10M 20M 30M 40M 50M); do
            for N in 10 25 50 100; do
                echo "running epoch $epoch with $B bytes and $N requests"
                /usr/bin/time -v -o out.txt ./build/randombytes_client -n $N -b $B -t 0 -q
                time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
                echo "$epoch $B $N $time" >> results_large.txt
                
                sleep 1 # small delay between requests
            done
        done
    done
}

main

cleanup