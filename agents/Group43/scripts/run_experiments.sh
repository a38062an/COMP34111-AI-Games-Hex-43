#!/bin/bash

# 1. Clean up old logs
echo "Cleaning up old logs..."
mkdir -p results
rm -f results/logs_optimized.txt results/logs_unoptimized.txt results/logs_pool_only.txt

# 2. Build executables
echo "Building executables..."
# Ensure we are in the project root (parent of scripts/)
cd "$(dirname "$0")/.."
cd src
make Experiment ExperimentBase ExperimentNoTT
cd ..

# 3. Run benchmarks in parallel
echo "Running benchmarks in parallel (100 games)..."
(./src/Experiment > results/logs_optimized.txt) &
PID1=$!
(./src/ExperimentBase > results/logs_unoptimized.txt) &
PID2=$!
(./src/ExperimentNoTT > results/logs_pool_only.txt) &
PID3=$!

# Wait for all to finish
wait $PID1 $PID2 $PID3
echo "Benchmarks Complete."

# 4. Generate Plots
echo "Generating Plots..."
python3 scripts/plot_experiments.py

# 5. Update Artifacts
echo "Updating Artifacts..."
cp plots/*.png /Users/anthonynguyen/.gemini/antigravity/brain/9cf88d4b-a692-4297-97d2-171160767d41/

echo "Done! Check plots/ directory and artifacts."
