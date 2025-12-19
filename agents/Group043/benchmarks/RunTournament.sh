#!/bin/bash

# 1. Navigate to Project Root (3 levels up from this script)
# This ensures we are at the top level for git commands and python execution
cd "$(dirname "$0")/../../.." || exit 1

# 2. Configuration
GAMES=${1:-50}        # Default to 50 games if not specified
AGENT_DIR="agents/Group43"

# 3. Store Current State (Safety Mechanism)
CURRENT_BRANCH=$(git branch --show-current)
HAS_CHANGES=$(git status --porcelain)

echo "=== Automated Build Sequence ==="
echo "Current branch: $CURRENT_BRANCH"

# If there are uncommitted changes, stash them to prevent conflicts
if [ -n "$HAS_CHANGES" ]; then
    echo "Uncommitted changes detected. Stashing..."
    git stash push -m "Auto-stash by RunTournament.sh"
fi

# ---------------------------------------------------------
# 4. Build Baseline Agent (from 'main')
# ---------------------------------------------------------
echo "--- Step 1: Building Baseline (CppAgent) on 'main' ---"
git checkout main || exit 1

# Clean and Build
make -C "$AGENT_DIR" clean
make -C "$AGENT_DIR"

# ---------------------------------------------------------
# 5. Build Experimental Agent (from 'dev')
# ---------------------------------------------------------
echo "--- Step 2: Building Experimental (DevAgent) on 'dev' ---"
git checkout dev || exit 1

# Note: We do NOT clean here, or we would lose the CppAgent we just built.
# We explicitly run the 'dev' target which builds DevAgent.
make -C "$AGENT_DIR" dev

# ---------------------------------------------------------
# 6. Restore Original State
# ---------------------------------------------------------
echo "--- Step 3: Restoring previous state ---"
git checkout "$CURRENT_BRANCH"

if [ -n "$HAS_CHANGES" ]; then
    echo "Restoring stashed changes..."
    git stash pop
fi

# ---------------------------------------------------------
# 7. Run Tournament
# ---------------------------------------------------------
if [ ! -f "$AGENT_DIR/bin/CppAgent" ] || [ ! -f "$AGENT_DIR/bin/DevAgent" ]; then
    echo "Error: Build failed. One or both binaries are missing."
    exit 1
fi

echo "=== Build Complete. Starting Tournament ($GAMES Games) ==="
python3 "$AGENT_DIR/benchmarks/RunTournament.py" "$GAMES"