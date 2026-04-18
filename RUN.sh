#!/bin/bash

set -e

echo "Starting build process..."

mkdir -p build
cd build

echo "Generating Makefiles..."
cmake ..

echo "Compiling..."
make -j$(nproc)

cd ..

echo "✅ Build successful!"
echo "------------------------------------------------"
echo "🌐 Starting Zero-Copy Load Balancer..."
echo "------------------------------------------------"

# 5. Execute the binary
./build/load_balancer

