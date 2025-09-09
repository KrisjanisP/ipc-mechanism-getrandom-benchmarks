# Unix Domain Socket Random Bytes Benchmark

This benchmark implements a client-server architecture using raw PF_UNIX (AF_UNIX) sockets for inter-process communication. It's designed to be compared against gRPC and D-Bus benchmarks for entropy distribution performance evaluation.

## Architecture

- **Server** (`socket_server`): Listens on a Unix domain socket and responds to requests for random bytes using the `getrandom()` system call
- **Client** (`socket_client`): Connects to the server and makes consecutive requests for random bytes with configurable parameters

## Protocol

The implementation uses a simple binary protocol:

### Request
```c
struct RandomBytesRequest {
    uint32_t num_bytes;  // Number of random bytes requested
};
```

### Response
```c
struct RandomBytesResponse {
    uint32_t actual_bytes;  // Number of bytes in the response
    // followed by actual_bytes of random data
};
```

## Building

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

## Usage

### Manual Testing

Start the server:
```bash
./build/socket_server
```

In another terminal, run the client:
```bash
./build/socket_client -n 1000 -b 32 -q
```

### Benchmark

Run the automated benchmark:
```bash
./benchmark.sh
```

This will:
1. Build the project if needed
2. Start the socket server in the background
3. Run the benchmark with the same parameters as gRPC and D-Bus benchmarks
4. Save results to `results.txt`
5. Clean up the server process and socket file

### Client Options

- `-n, --iterations NUM`: Number of socket calls to make (default: 1)
- `-b, --bytes NUM`: Number of bytes to retrieve per call (default: 10)
- `-t, --timeout MS`: Timeout in milliseconds (not implemented, compatibility only)
- `-l, --log`: Log output to stdout (default: enabled)
- `-q, --quiet`: Disable logging to stdout
- `-s, --socket PATH`: Socket path (default: `/tmp/randombytes_socket`)
- `-h, --help`: Show help message

### Server Options

- `-s, --socket PATH`: Socket path (default: `/tmp/randombytes_socket`)
- `-h, --help`: Show help message

## Benchmark Parameters

The benchmark runs with the same parameters as the gRPC and D-Bus benchmarks:
- **Epochs**: 20 repetitions
- **Byte sizes**: 1, 32, 1024 bytes per request
- **Iteration counts**: 100, 1000, 10000, 25000, 50000 requests

## Implementation Details

- Uses `AF_UNIX` sockets bound to filesystem paths
- Server handles one client at a time (no concurrent connections)
- Each client request opens a new socket connection
- Uses `getrandom()` system call for entropy generation
- Maximum request size limited to 1MB
- Includes proper error handling and cleanup

## Performance Characteristics

Unix domain sockets provide:
- Lower overhead than TCP/IP sockets
- Higher overhead than shared memory
- Built-in flow control and reliability
- No network stack involvement
- Direct kernel-to-kernel communication

This benchmark serves as a baseline for "raw socket" performance compared to higher-level IPC frameworks like gRPC and D-Bus.