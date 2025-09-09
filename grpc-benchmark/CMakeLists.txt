cmake_minimum_required(VERSION 3.16)
project(grpc_randombytes)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# Generate protobuf and gRPC files
get_filename_component(rb_proto "randombytes.proto" ABSOLUTE)
get_filename_component(rb_proto_path "${rb_proto}" PATH)

# Generated sources
set(rb_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/randombytes.pb.cc")
set(rb_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/randombytes.pb.h")
set(rb_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/randombytes.grpc.pb.cc")
set(rb_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/randombytes.grpc.pb.h")

add_custom_command(
    OUTPUT "${rb_proto_srcs}" "${rb_proto_hdrs}" "${rb_grpc_srcs}" "${rb_grpc_hdrs}"
    COMMAND $<TARGET_FILE:protobuf::protoc>
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${rb_proto_path}"
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         "${rb_proto}"
    DEPENDS "${rb_proto}")

# Include generated files and ensure installed includes are visible
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("$ENV{MY_INSTALL_DIR}/include")

# Create server executable
add_executable(randombytes_server
    randombytes_server.cc
    ${rb_proto_srcs}
    ${rb_grpc_srcs})

target_link_libraries(randombytes_server
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    absl::flags
    absl::flags_parse
    absl::log_initialize
    Threads::Threads)

# Create client executable
add_executable(randombytes_client
    randombytes_client.cc
    ${rb_proto_srcs}
    ${rb_grpc_srcs})

target_link_libraries(randombytes_client
    gRPC::grpc++
    protobuf::libprotobuf
    absl::flags
    absl::flags_parse
    Threads::Threads)