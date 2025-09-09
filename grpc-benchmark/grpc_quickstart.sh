#!/bin/bash

# gRPC C++ Quickstart Script
# Following: https://grpc.io/docs/languages/cpp/quickstart/

set -e  # Exit on any error

echo "=== gRPC C++ Quickstart Setup Script ==="

# Setup environment variables
export MY_INSTALL_DIR=$HOME/.local
echo "Setting MY_INSTALL_DIR to: $MY_INSTALL_DIR"

# Ensure the directory exists
mkdir -p $MY_INSTALL_DIR

# Add the local bin folder to PATH
export PATH="$MY_INSTALL_DIR/bin:$PATH"
echo "Updated PATH to include: $MY_INSTALL_DIR/bin"

echo ""
echo "=== Step 1: Installing Dependencies ==="

# Check if we're on Ubuntu/Debian
if command -v apt &> /dev/null; then
    echo "Installing dependencies using apt..."
    sudo apt update
    sudo apt install -y build-essential autoconf libtool pkg-config cmake
elif command -v brew &> /dev/null; then
    echo "Installing dependencies using brew..."
    brew install cmake autoconf automake libtool pkg-config
else
    echo "Warning: Could not detect package manager. Please ensure you have:"
    echo "  - cmake (version 3.16 or later)"
    echo "  - build-essential (or equivalent)"
    echo "  - autoconf, libtool, pkg-config"
fi

# Check cmake version
echo ""
echo "Checking cmake version..."
cmake --version

echo ""
echo "=== Step 2: Building and Installing gRPC ==="

# Navigate to grpc directory
cd grpc

# Create build directory
mkdir -p cmake/build
cd cmake/build

echo "Configuring gRPC build..."
cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..

echo "Building gRPC (this may take a while)..."
make -j$(nproc)

echo "Installing gRPC..."
make install

# Go back to repo root
cd ../..

echo ""
echo "=== Step 3: Building the Hello World Example ==="

# Navigate to the helloworld example
cd examples/cpp/helloworld

# Create build directory for the example
mkdir -p cmake/build
cd cmake/build

echo "Configuring helloworld example..."
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..

echo "Building helloworld example..."
make -j$(nproc)

echo ""
echo "=== Step 4: Testing the Example ==="

# Check if binaries were created
if [[ -f "./greeter_server" && -f "./greeter_client" ]]; then
    echo "✅ Build successful! Binaries created:"
    echo "  - greeter_server"
    echo "  - greeter_client"
    
    echo ""
    echo "=== Step 5: Running the Example ==="
    echo "Starting the server in the background..."
    
    # Start server in background
    ./greeter_server &
    SERVER_PID=$!
    
    # Wait a moment for server to start
    sleep 2
    
    echo "Running the client..."
    ./greeter_client
    
    # Clean up: kill the server
    echo ""
    echo "Stopping the server..."
    kill $SERVER_PID 2>/dev/null || true
    
    echo ""
    echo "🎉 gRPC C++ quickstart completed successfully!"
    echo ""
    echo "To run manually:"
    echo "  1. Start server: cd $(pwd) && ./greeter_server"
    echo "  2. In another terminal, run client: cd $(pwd) && ./greeter_client"
    
else
    echo "❌ Build failed - binaries not found"
    exit 1
fi

echo ""
echo "=== Next Steps ==="
echo "You can now:"
echo "1. Modify the .proto file in examples/protos/helloworld.proto"
echo "2. Rebuild with: make -j$(nproc)"
echo "3. Update the server and client code as needed"
echo "4. Explore more examples in the examples/ directory"