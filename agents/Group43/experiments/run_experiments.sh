#!/bin/bash

# ==============================================================================
# Script Location: agents/Group43/experiments/run_experiments.sh
# Purpose: Compiles the agent, runs benchmarks, and generates graphs.
# ==============================================================================

# 1. Setup Environment
# Navigate to Project Root (Parent of the 'experiments' folder)
cd "$(dirname "$0")/.."
echo "Project Root: $(pwd)"

# Create output directories
mkdir -p results
mkdir -p bin
mkdir -p plots

# 2. Clean up old data
echo "[1/4] Cleaning up old results..."
rm -f results/optimized.csv results/baseline.csv results/nott.csv

# 3. Build executables
echo "[2/4] Compiling experiments..."
# Run make from the root, targeting the binaries defined in the Makefile
make bin/Experiment bin/ExperimentBase bin/ExperimentNoTT

# Safety Check: Ensure compilation worked
if [ ! -f bin/Experiment ]; then
    echo "Error: Build failed. bin/Experiment not found."
    exit 1
fi

# 4. Run benchmarks in parallel
echo "[3/4] Running benchmarks in parallel..."

echo "   > Running Optimized (Ferrari)..."
(./bin/Experiment > results/optimized.csv) &
PID1=$!

echo "   > Running Baseline (Honda Civic)..."
(./bin/ExperimentBase > results/baseline.csv) &
PID2=$!

echo "   > Running Intermediate (No TT)..."
(./bin/ExperimentNoTT > results/nott.csv) &
PID3=$!

# Wait for background processes
wait $PID1 $PID2 $PID3
echo "   Benchmarks Complete."

# 5. Generate Plots
echo "[4/4] Generating Plots..."

PLOT_SCRIPT="experiments/plot_experiments.py"

python3 "$PLOT_SCRIPT" results/optimized.csv results/baseline.csv results/nott.csv

echo "---------------------------------------------------"
echo "Done! Graphs generated in 'plots/' directory."
echo "---------------------------------------------------"