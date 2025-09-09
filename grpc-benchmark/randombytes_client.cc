/*
 * gRPC Random Bytes Client
 * Requests random bytes from the server with configurable parameters
 */

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <getopt.h>
#include <cstdio>
#include <cstdlib>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "randombytes.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using randombytes::RandomBytesService;
using randombytes::RandomBytesRequest;
using randombytes::RandomBytesReply;

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

class RandomBytesClient {
 public:
  RandomBytesClient(std::shared_ptr<Channel> channel)
      : stub_(RandomBytesService::NewStub(channel)) {}

  // Request random bytes from the server
  bool GetRandomBytes(uint32_t num_bytes, int timeout_ms, bool log_output) {
    RandomBytesRequest request;
    request.set_num_bytes(num_bytes);

    RandomBytesReply reply;
    ClientContext context;
    
    // Set timeout if specified
    if (timeout_ms > 0) {
      auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout_ms);
      context.set_deadline(deadline);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    
    // The actual RPC
    Status status = stub_->GetRandomBytes(&context, request, &reply);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    if (status.ok()) {
      if (log_output) {
        std::cout << "Received " << reply.actual_bytes() << " random bytes in " 
                  << duration.count() << " μs" << std::endl;
        
        // Print first few bytes as hex for verification
        const std::string& data = reply.data();
        std::cout << "First bytes (hex): ";
        for (size_t i = 0; i < std::min(static_cast<size_t>(16), data.size()); ++i) {
          printf("%02x ", static_cast<unsigned char>(data[i]));
        }
        if (data.size() > 16) {
          std::cout << "...";
        }
        std::cout << std::endl;
      }
      return true;
    } else {
      if (log_output) {
        std::cout << "RPC failed: " << status.error_code() << ": " 
                  << status.error_message() << std::endl;
      }
      return false;
    }
  }

 private:
  std::unique_ptr<RandomBytesService::Stub> stub_;
};

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Options:\n");
    printf("  -n, --iterations NUM    Number of gRPC calls to make (default: 1)\n");
    printf("  -b, --bytes NUM         Number of bytes to retrieve per call (default: 10)\n");
    printf("  -t, --timeout MS        Timeout in milliseconds (default: 0 = no timeout)\n");
    printf("  -l, --log               Log output to stdout (default: enabled)\n");
    printf("  -q, --quiet             Disable logging to stdout\n");
    printf("  -s, --server ADDRESS    Server address (default: localhost:50051)\n");
    printf("  -h, --help              Show this help message\n");
}

int main(int argc, char** argv) {
    int iterations = 1;
    int bytes = 10;
    int timeout_ms = 0;
    bool log_output = true;
    std::string server_address = "localhost:50051";
    
    static struct option long_options[] = {
        {"iterations", required_argument, 0, 'n'},
        {"bytes", required_argument, 0, 'b'},
        {"timeout", required_argument, 0, 't'},
        {"log", no_argument, 0, 'l'},
        {"quiet", no_argument, 0, 'q'},
        {"server", required_argument, 0, 's'},
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
                server_address = optarg;
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
        std::cout << "gRPC Random Bytes Client" << std::endl;
        std::cout << "Server: " << server_address << std::endl;
        std::cout << "Iterations: " << iterations << std::endl;
        std::cout << "Bytes per call: " << bytes << std::endl;
        std::cout << "Timeout: " << (timeout_ms > 0 ? std::to_string(timeout_ms) + "ms" : "none") << std::endl;
        std::cout << "---" << std::endl;
    }

    // Create client with 100MB message size limit
    grpc::ChannelArguments args;
    const int max_message_size = 100 * 1024 * 1024; // 100MB
    args.SetMaxReceiveMessageSize(max_message_size);
    args.SetMaxSendMessageSize(max_message_size);
    
    RandomBytesClient client(
        grpc::CreateCustomChannel(server_address, grpc::InsecureChannelCredentials(), args));

    int successful_calls = 0;
    auto total_start = std::chrono::high_resolution_clock::now();
    
    // Make the specified number of calls
    for (int i = 0; i < iterations; ++i) {
        if (log_output && iterations > 1) {
            std::cout << "Call " << (i + 1) << "/" << iterations << ": ";
        }
        
        if (client.GetRandomBytes(bytes, timeout_ms, log_output)) {
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