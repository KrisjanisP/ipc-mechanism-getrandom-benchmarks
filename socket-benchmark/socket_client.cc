/*
 * Unix Domain Socket Random Bytes Client
 * Requests random bytes from the server with configurable parameters
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <getopt.h>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// Protocol definition (must match server)
struct RandomBytesRequest {
    uint32_t num_bytes;
};

struct RandomBytesResponse {
    uint32_t actual_bytes;
    // followed by actual_bytes of data
};

const char* SOCKET_PATH = "/tmp/randombytes_socket";

class SocketRandomBytesClient {
private:
    std::string socket_path_;

public:
    SocketRandomBytesClient(const std::string& socket_path) 
        : socket_path_(socket_path) {}

    // Request random bytes from the server
    bool GetRandomBytes(uint32_t num_bytes, bool log_output) {
        // Create socket
        int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            if (log_output) {
                std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
            }
            return false;
        }

        // Connect to server
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

        auto start_time = std::chrono::high_resolution_clock::now();

        if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            if (log_output) {
                std::cerr << "Failed to connect to server: " << strerror(errno) << std::endl;
            }
            close(sock_fd);
            return false;
        }

        // Send request
        RandomBytesRequest request;
        request.num_bytes = num_bytes;

        ssize_t bytes_sent = send(sock_fd, &request, sizeof(request), 0);
        if (bytes_sent != sizeof(request)) {
            if (log_output) {
                std::cerr << "Failed to send request: " << strerror(errno) << std::endl;
            }
            close(sock_fd);
            return false;
        }

        // Receive response header
        RandomBytesResponse response;
        ssize_t bytes_received = recv(sock_fd, &response, sizeof(response), 0);
        if (bytes_received != sizeof(response)) {
            if (log_output) {
                std::cerr << "Failed to receive response header: " << strerror(errno) << std::endl;
            }
            close(sock_fd);
            return false;
        }

        // Receive response data
        std::vector<uint8_t> data;
        if (response.actual_bytes > 0) {
            data.resize(response.actual_bytes);
            size_t total_received = 0;
            while (total_received < response.actual_bytes) {
                bytes_received = recv(sock_fd, data.data() + total_received, 
                                    response.actual_bytes - total_received, 0);
                if (bytes_received <= 0) {
                    if (log_output) {
                        std::cerr << "Failed to receive response data: " << strerror(errno) << std::endl;
                    }
                    close(sock_fd);
                    return false;
                }
                total_received += bytes_received;
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        close(sock_fd);

        if (log_output) {
            std::cout << "Received " << response.actual_bytes << " bytes in " 
                      << duration.count() << " μs";
            
            // Print first few bytes if requested small amount
            if (response.actual_bytes <= 32 && response.actual_bytes > 0) {
                std::cout << " [";
                for (size_t i = 0; i < std::min(static_cast<size_t>(response.actual_bytes), size_t(8)); ++i) {
                    if (i > 0) std::cout << " ";
                    std::cout << std::hex << static_cast<int>(data[i]);
                }
                if (response.actual_bytes > 8) {
                    std::cout << " ...";
                }
                std::cout << "]" << std::dec;
            }
            std::cout << std::endl;
        }

        return true;
    }
};

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -n, --iterations NUM    Number of socket calls to make (default: 1)\n");
    printf("  -b, --bytes NUM         Number of bytes to retrieve per call (default: 10)\n");
    printf("  -t, --timeout MS        Timeout in milliseconds (default: 0 = no timeout)\n");
    printf("  -l, --log               Log output to stdout (default: enabled)\n");
    printf("  -q, --quiet             Disable logging to stdout\n");
    printf("  -s, --socket PATH       Socket path (default: %s)\n", SOCKET_PATH);
    printf("  -h, --help              Show this help message\n");
}

int main(int argc, char** argv) {
    int iterations = 1;
    int bytes = 10;
    int timeout_ms = 0; // Note: timeout not implemented for sockets in this simple version
    bool log_output = true;
    std::string socket_path = SOCKET_PATH;
    
    static struct option long_options[] = {
        {"iterations", required_argument, 0, 'n'},
        {"bytes", required_argument, 0, 'b'},
        {"timeout", required_argument, 0, 't'},
        {"log", no_argument, 0, 'l'},
        {"quiet", no_argument, 0, 'q'},
        {"socket", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    int option_index = 0;
    
    while ((c = getopt_long(argc, argv, "n:b:t:lqs:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'n':
                iterations = atoi(optarg);
                if (iterations <= 0) {
                    fprintf(stderr, "Error: iterations must be positive\n");
                    return 1;
                }
                break;
            case 'b':
                bytes = atoi(optarg);
                if (bytes <= 0) {
                    fprintf(stderr, "Error: bytes must be positive\n");
                    return 1;
                }
                break;
            case 't':
                timeout_ms = atoi(optarg);
                if (timeout_ms < 0) {
                    fprintf(stderr, "Error: timeout must be non-negative\n");
                    return 1;
                }
                break;
            case 'l':
                log_output = true;
                break;
            case 'q':
                log_output = false;
                break;
            case 's':
                socket_path = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case '?':
                print_usage(argv[0]);
                return 1;
            default:
                abort();
        }
    }
    
    if (log_output) {
        std::cout << "Unix Socket Random Bytes Client" << std::endl;
        std::cout << "Socket: " << socket_path << std::endl;
        std::cout << "Iterations: " << iterations << std::endl;
        std::cout << "Bytes per call: " << bytes << std::endl;
        std::cout << "Timeout: " << (timeout_ms > 0 ? std::to_string(timeout_ms) + "ms (not implemented)" : "none") << std::endl;
        std::cout << "---" << std::endl;
    }

    // Create client
    SocketRandomBytesClient client(socket_path);

    int successful_calls = 0;
    auto total_start = std::chrono::high_resolution_clock::now();
    
    // Make the specified number of calls
    for (int i = 0; i < iterations; ++i) {
        if (log_output && iterations > 1) {
            std::cout << "Call " << (i + 1) << "/" << iterations << ": ";
        }
        
        if (client.GetRandomBytes(bytes, log_output)) {
            successful_calls++;
        }
    }
    
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start);
    
    if (log_output) {
        std::cout << "---" << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "Successful calls: " << successful_calls << "/" << iterations << std::endl;
        std::cout << "Total time: " << total_duration.count() << " μs" << std::endl;
        if (iterations > 1) {
            std::cout << "Average time per call: " << (total_duration.count() / iterations) << " μs" << std::endl;
        }
        std::cout << "Success rate: " << (100.0 * successful_calls / iterations) << "%" << std::endl;
    }

    return (successful_calls == iterations) ? 0 : 1;
}