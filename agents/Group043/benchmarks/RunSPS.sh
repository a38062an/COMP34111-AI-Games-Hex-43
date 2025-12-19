#!/bin/bash

# Navigate to Project Root
cd "$(dirname "$0")/.."

echo "--- Hex SPS Benchmark Runner ---"

echo "[1/2] Compiling SPS Benchmark..."
make bin/SPS

# Safety Check
if [ ! -f bin/SPS ]; then
    echo "Error: Compilation failed."
    exit 1
fi

# 2. Run the Benchmark
echo "[2/2] Running SPS Benchmark..."
echo ""
./bin/SPS