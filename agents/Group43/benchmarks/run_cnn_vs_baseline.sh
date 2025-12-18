#!/bin/bash
# Run CNN Agent vs Baseline Agent (No CNN)

echo "=================================================="
echo "ROUND 1: CNN Agent (P1) vs Baseline (P2)"
echo "=================================================="

# Note: We must run from project root because of python module imports
# Navigate to project root relative to this script
cd "$(dirname "$0")/../../.."

# Run Round 1
# P1 (Alice)=CNN, P2 (Bob)=Baseline
python3 Hex.py -p1 "agents.Group43.Agents Group43Agent" -p2 "agents.Group43.Agents Group43Baseline" -b 11 -v > round1.log 2>&1
cat round1.log

echo ""
echo "--------------------------------------------------"
if grep -q "winner,Alice,WIN" round1.log; then
    echo "🏆 ROUND 1 WINNER: CNN AGENT (Alice)"
elif grep -q "winner,Bob,WIN" round1.log; then
    echo "🏆 ROUND 1 WINNER: BASELINE AGENT (Bob)"
else
    echo "⚠️ ROUND 1 RESULT: Indeterminate (Check output)"
fi
echo "--------------------------------------------------"

echo ""
echo "=================================================="
echo "ROUND 2: Baseline (P1) vs CNN Agent (P2)"
echo "=================================================="

# Run Round 2
# P1 (Alice)=Baseline, P2 (Bob)=CNN
python3 Hex.py -p1 "agents.Group43.Agents Group43Baseline" -p2 "agents.Group43.Agents Group43Agent" -b 11 -v > round2.log 2>&1
cat round2.log

echo ""
echo "--------------------------------------------------"
if grep -q "winner,Alice,WIN" round2.log; then
    echo "🏆 ROUND 2 WINNER: BASELINE AGENT (Alice)"
elif grep -q "winner,Bob,WIN" round2.log; then
    echo "🏆 ROUND 2 WINNER: CNN AGENT (Bob)"
else
    echo "⚠️ ROUND 2 RESULT: Indeterminate (Check output)"
fi
echo "--------------------------------------------------"

# Cleanup
rm round1.log round2.log

echo "Done."
