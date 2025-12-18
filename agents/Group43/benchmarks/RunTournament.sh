#!/bin/bash

# 1. Navigate to Project Root (3 levels up from this script)
# This ensures Agents.py can find the binaries in 'agents/Group43/bin'
cd "$(dirname "$0")/../../.." || exit 1

# 2. Configuration
GAMES=${1:-50} # Default to 50 games if not specified

# 3. Validation (Optional but recommended)
if [ ! -f "agents/Group43/bin/CppAgent" ] || [ ! -f "agents/Group43/bin/DevAgent" ]; then
    echo "Error: Binaries missing. Please run 'make' (Main) and 'make dev' (Feature)."
    exit 1
fi

# 4. Run
echo "Starting Tournament ($GAMES Games)..."
python3 agents/Group43/benchmarks/RunTournament.py "$GAMES"