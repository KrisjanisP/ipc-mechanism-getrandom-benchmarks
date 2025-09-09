#! /bin/bash
set -e # exit on error
# set -x # print commands

# Kill any existing process listening on port 50051
lsof -ti:50051 | xargs -r kill -9

# Cleanup function to kill server on exit
cleanup() {
    echo "Cleaning up..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

./build/randombytes_server &
SERVER_PID=$!

sleep 3

for epoch in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    for B in 1 32 1024; do
        for N in 100 1000 10000 25000; do
            /usr/bin/time -v -o out.txt ./build/randombytes_client -n $N -b $B -t 0 -q
            time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
            echo "$epoch $B $N $time" >> results.txt
            # rm out.txt
        done
    done
done

kill $SERVER_PID