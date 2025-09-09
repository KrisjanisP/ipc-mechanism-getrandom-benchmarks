#!/bin/bash
set -e # exit on error
# set -x # print commands

# Socket path
SOCKET_PATH="/tmp/randombytes_socket"

# Remove any existing socket file
rm -f "$SOCKET_PATH"

# Kill any existing socket server processes
pkill -f socket_server || true

# Cleanup function to kill server on exit
cleanup() {
    echo "Cleaning up..."
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    rm -f "$SOCKET_PATH"
}
trap cleanup EXIT

# Build the project if build directory doesn't exist
if [ ! -d "build" ]; then
    echo "Building socket benchmark..."
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    cd ..
fi

# Start the socket server
./build/socket_server -s "$SOCKET_PATH" &
SERVER_PID=$!

# Wait for server to start up
sleep 2

# Check if server is still running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Error: Server failed to start"
    exit 1
fi

echo "Running socket benchmark..."
echo "Socket path: $SOCKET_PATH"
echo "Server PID: $SERVER_PID"
echo ""

# Run the benchmark with same parameters as gRPC and D-Bus benchmarks
for epoch in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
    for B in 1 32 1024; do
        for N in 100 1000 10000 25000 50000 100000; do
            echo "Running epoch $epoch: $N iterations, $B bytes per call"
            /usr/bin/time -v -o out.txt ./build/socket_client -n $N -b $B -t 0 -q -s "$SOCKET_PATH"
            time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
            echo "$epoch $B $N $time" >> results.txt
            rm -f out.txt
        done
    done
done

echo "Benchmark completed. Results saved to results.txt"

# Server will be killed by cleanup function