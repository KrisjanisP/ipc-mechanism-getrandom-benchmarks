/*
 * Unix Domain Socket Random Bytes Server
 * Uses getrandom() syscall to generate truly random bytes
 * Communicates via PF_UNIX (AF_UNIX) sockets
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sys/random.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <cstring>
#include <cerrno>

// Protocol definition
struct RandomBytesRequest {
    uint32_t num_bytes;
};

struct RandomBytesResponse {
    uint32_t actual_bytes;
    // followed by actual_bytes of data
};

const char* SOCKET_PATH = "/tmp/randombytes_socket";
volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -s, --socket PATH       Socket path (default: %s)\n", SOCKET_PATH);
    printf("  -h, --help              Show this help message\n");
}

// Generate random bytes using getrandom syscall
bool generate_random_bytes(std::vector<uint8_t>& buffer, size_t num_bytes) {
    if (num_bytes == 0) {
        return true;
    }
    
    buffer.resize(num_bytes);
    
    ssize_t result = getrandom(buffer.data(), num_bytes, 0);
    if (result < 0) {
        std::cerr << "getrandom failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    if (static_cast<size_t>(result) != num_bytes) {
        std::cerr << "getrandom returned fewer bytes than requested: " 
                  << result << " vs " << num_bytes << std::endl;
        return false;
    }
    
    return true;
}

bool handle_client(int client_fd) {
    RandomBytesRequest request;
    
    // Read request
    ssize_t bytes_read = recv(client_fd, &request, sizeof(request), 0);
    if (bytes_read != sizeof(request)) {
        std::cerr << "Failed to read request: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Validate request - no size limits imposed
    
    // Generate random bytes
    std::vector<uint8_t> random_data;
    if (!generate_random_bytes(random_data, request.num_bytes)) {
        return false;
    }
    
    // Send response header
    RandomBytesResponse response;
    response.actual_bytes = request.num_bytes;
    
    ssize_t bytes_sent = send(client_fd, &response, sizeof(response), 0);
    if (bytes_sent != sizeof(response)) {
        std::cerr << "Failed to send response header: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Send response data
    if (request.num_bytes > 0) {
        size_t total_sent = 0;
        while (total_sent < request.num_bytes) {
            bytes_sent = send(client_fd, random_data.data() + total_sent, 
                            request.num_bytes - total_sent, 0);
            if (bytes_sent <= 0) {
                std::cerr << "Failed to send response data: " << strerror(errno) << std::endl;
                return false;
            }
            total_sent += bytes_sent;
        }
    }
    
    return true;
}

int main(int argc, char *argv[]) {
    std::string socket_path = SOCKET_PATH;
    
    // Command line option parsing
    static struct option long_options[] = {
        {"socket", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "s:h", long_options, NULL)) != -1) {
        switch (c) {
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
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return 1;
    }
    
    // Remove existing socket file if it exists
    unlink(socket_path.c_str());
    
    // Bind socket
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        close(server_fd);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        close(server_fd);
        unlink(socket_path.c_str());
        return 1;
    }
    
    std::cout << "Socket server listening on: " << socket_path << std::endl;
    
    // Main server loop
    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int select_result = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal
            }
            std::cerr << "Select failed: " << strerror(errno) << std::endl;
            break;
        }
        
        if (select_result == 0) {
            continue; // Timeout
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            if (client_fd < 0) {
                std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
                continue;
            }
            
            // Handle client request
            handle_client(client_fd);
            close(client_fd);
        }
    }
    
    std::cout << "Server shutting down..." << std::endl;
    
    // Cleanup
    close(server_fd);
    unlink(socket_path.c_str());
    
    return 0;
}