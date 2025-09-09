/*
 * gRPC Random Bytes Server
 * Uses getrandom() syscall to generate truly random bytes
 */

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sys/random.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"

#include "randombytes.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using randombytes::RandomBytesService;
using randombytes::RandomBytesRequest;
using randombytes::RandomBytesReply;

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

// Logic and data behind the server's behavior.
class RandomBytesServiceImpl final : public RandomBytesService::Service {
  Status GetRandomBytes(ServerContext* context, const RandomBytesRequest* request,
                       RandomBytesReply* reply) override {
    uint32_t num_bytes = request->num_bytes();
    
    // Limit the maximum number of bytes to prevent abuse
    const uint32_t MAX_BYTES = 1024 * 1024; // 1MB limit
    if (num_bytes > MAX_BYTES) {
      return Status(grpc::StatusCode::INVALID_ARGUMENT, 
                   "Requested too many bytes (max: " + std::to_string(MAX_BYTES) + ")");
    }
    
    if (num_bytes == 0) {
      reply->set_actual_bytes(0);
      return Status::OK;
    }
    
    // Allocate buffer for random bytes
    std::vector<uint8_t> buffer(num_bytes);
    
    // Use getrandom() syscall to get truly random bytes
    ssize_t result = getrandom(buffer.data(), num_bytes, GRND_NONBLOCK);
    
    if (result < 0) {
      return Status(grpc::StatusCode::INTERNAL, 
                   "Failed to generate random bytes: " + std::string(strerror(errno)));
    }
    
    // Set the random bytes in the reply
    reply->set_data(buffer.data(), result);
    reply->set_actual_bytes(static_cast<uint32_t>(result));
    
    return Status::OK;
  }
};

void RunServer(uint16_t port) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  RandomBytesServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "RandomBytes Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  RunServer(absl::GetFlag(FLAGS_port));
  return 0;
}