#!/bin/bash

# Navigate to Project Root
cd "$(dirname "$0")/.."

echo "--- Hex Benchmark Runner ---"

# 1. Compile ONLY the standard optimized experiment
# We skip ExperimentBase and ExperimentNoTT
echo "[1/2] Compiling Benchmark..."
make bin/Experiment

# Safety Check
if [ ! -f bin/Experiment ]; then
    echo "Error: Compilation failed."
    exit 1
fi

# 2. Run the Benchmark
echo "[2/2] Running Benchmark..."
echo ""
./bin/Experiment