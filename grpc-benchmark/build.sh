#!/bin/bash

set -e

export MY_INSTALL_DIR=$HOME/.local
export PATH="$MY_INSTALL_DIR/bin:$PATH"
export PKG_CONFIG_PATH="$MY_INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

mkdir -p build
cd build

echo "Configuring build..."
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ..

echo "Building..."
make -j$(nproc)