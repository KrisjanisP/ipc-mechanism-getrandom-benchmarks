#!/bin/bash
set -e # exit on error
# set -x # print commands

echo "Unix Socket Large Request Benchmark"
echo "Testing large request sizes: 1MB, 10MB, 50MB, 100MB, 200MB"
echo "Running 3 epochs with single requests per test"
echo ""

# Socket path
SOCKET_PATH="/tmp/randombytes_socket_large"

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

echo "Running large socket benchmark..."
echo "Socket path: $SOCKET_PATH"
echo "Server PID: $SERVER_PID"
echo ""

# Large request sizes in bytes
# 1MB = 1048576, 10MB = 10485760, 50MB = 52428800, 100MB = 104857600, 200MB = 209715200
LARGE_SIZES=(1048576 10485760 52428800 104857600 209715200)
SIZE_LABELS=("1MB" "10MB" "50MB" "100MB" "200MB")

for epoch in 1 2 3; do
    for i in "${!LARGE_SIZES[@]}"; do
        B=${LARGE_SIZES[$i]}
        LABEL=${SIZE_LABELS[$i]}
        echo "Running epoch $epoch: 1 request, $LABEL ($B bytes)"
        
        # Single request for large sizes due to memory and time constraints
        /usr/bin/time -v -o out.txt ./build/socket_client -n 1 -b $B -t 0 -q -s "$SOCKET_PATH"
        time=$(cat out.txt | grep "m:ss): " | cut -c 47-)
        echo "$epoch $B 1 $time" >> results_large.txt
        rm -f out.txt
        
        # Add a small delay between large requests
        sleep 1
    done
done

echo ""
echo "Large benchmark completed. Results saved to results_large.txt"
echo "Format: epoch bytes_per_request num_requests wall_time"

# Server will be killed by cleanup function