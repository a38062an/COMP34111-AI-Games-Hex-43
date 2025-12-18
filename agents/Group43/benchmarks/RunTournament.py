import subprocess
import re
import sys
import time

# Configuration
NUM_GAMES = 20        # Total pairs of games (so 2 * 20 = 40 games)
TIME_LIMIT_MS = 1000  # 1000ms per move for better search depth
BOARD_SIZE = 11

def run_game(cnn_color, baseline_color):
    """
    Runs a single game.
    Returns 'CNN' if CNN won, 'Baseline' if Baseline won, 'Error' otherwise.
    """
    
    # Define Agents
    # Note: We need to pass --time arg. But Hex.py wraps the agents.
    # We can modify Agents.py to check for an environment variable or just modify the class definition temporarily?
    # Actually, simpler: Use the Group43Agent/Baseline defined in Agents.py, but update Agents.py to accept --time?
    # Wait, Agents.py calls the executable. It constructs the command list.
    # It hardcodes args? No, it takes extra_args list.
    # But Hex.py instantiates agents without arguments usually (just color).
    # Ah, Hex.py allows parameter passing? "module class" string.
    
    # WORKAROUND: We will construct huge command strings that Hex.py parses.
    # Hex.py parses: -p1 "path.to.module ClassName"
    # It doesn't seemingly pass extra args to constructor.
    
    # BUT: We modified CppAgent to take --time.
    # We updated Agents.py:
    # class Group43Baseline(Group43AgentBase): ... extra_args=["--no-cnn"]
    
    # We need to modify Agents.py AGAIN to allow injecting time limit?
    # Or better: We just add a "Fast" agent to Agents.py that includes --time?
    
    # Let's assume we update Agents.py to have "Fast" variants.
    # OR: We just modify Agents.py locally right before running? No that's hacky.
    
    pass 

# Actually, let's create a temporary AgentsFast.py that has the time limit baked in.
AGENTS_PY_CONTENT = f"""
from agents.Group43.Agents import Group43AgentBase
from src.Colour import Colour

class FastCNNAgent(Group43AgentBase):
    def __init__(self, colour: Colour):
        super().__init__(colour, binary_name="bin/CppAgent", extra_args=["--time", "{TIME_LIMIT_MS}"])

class FastBaselineAgent(Group43AgentBase):
    def __init__(self, colour: Colour):
        super().__init__(colour, binary_name="bin/CppAgent", extra_args=["--no-cnn", "--time", "{TIME_LIMIT_MS}"])
"""

with open("agents/Group43/AgentsFast.py", "w") as f:
    f.write(AGENTS_PY_CONTENT)

# Now we run Hex.py using these new agents
def run_match(game_id):
    print(f"Game {game_id}: CNN (Red) vs Baseline (Blue)...", end="", flush=True)
    p1 = "agents.Group43.AgentsFast FastCNNAgent"
    p2 = "agents.Group43.AgentsFast FastBaselineAgent"
    
    # Remove -v to reduce noise, though it might be needed for the 'winner' line if it's a log.
    # Actually, the 'winner,X,WIN' line is usually a direct print.
    # Let's keep -v but search both streams.
    cmd = ["python3", "Hex.py", "-p1", p1, "-p2", p2, "-b", str(BOARD_SIZE), "-v"]
    
    try:
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        # Combine output
        combined_output = result.stdout + result.stderr
        
        if "winner,Alice,WIN" in combined_output:
            print(" CNN Won!")
            return "CNN"
        elif "winner,Bob,WIN" in combined_output:
            print(" Baseline Won.")
            return "Baseline"
        else:
            print(" Error/Indeterminate")
            # Only print tail of output to avoid flooding
            print("DEBUG: Stderr tail:", result.stderr[-500:])
            return "Error"
    except subprocess.TimeoutExpired:
        print(" Timeout")
        return "Timeout"

def run_match_swap(game_id):
    print(f"Game {game_id} (Swap): Baseline (Red) vs CNN (Blue)...", end="", flush=True)
    p1 = "agents.Group43.AgentsFast FastBaselineAgent"
    p2 = "agents.Group43.AgentsFast FastCNNAgent"
    
    cmd = ["python3", "Hex.py", "-p1", p1, "-p2", p2, "-b", str(BOARD_SIZE), "-v"]
    
    try:
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        combined_output = result.stdout + result.stderr
        
        if "winner,Alice,WIN" in combined_output:
            print(" Baseline Won.") # P1 is baseline
            return "Baseline"
        elif "winner,Bob,WIN" in combined_output:
            print(" CNN Won!") # P2 is CNN
            return "CNN"
        else:
            print(" Error/Indeterminate")
            print("DEBUG: Stderr tail:", result.stderr[-500:])
            return "Error"
    except subprocess.TimeoutExpired:
        print(" Timeout")
        return "Timeout"


# Main Loop
print(f"Starting {NUM_GAMES*2} games tournament (Time Limit: {TIME_LIMIT_MS}ms per move)")
cnn_wins = 0
baseline_wins = 0
errors = 0

for i in range(1, NUM_GAMES + 1):
    res1 = run_match(i)
    if res1 == "CNN": cnn_wins += 1
    elif res1 == "Baseline": baseline_wins += 1
    else: errors += 1
    
    res2 = run_match_swap(i)
    if res2 == "CNN": cnn_wins += 1
    elif res2 == "Baseline": baseline_wins += 1
    else: errors += 1

total_valid = cnn_wins + baseline_wins
print("\n================ FINAL RESULTS ================")
print(f"Total Games Played: {NUM_GAMES * 2}")
print(f"CNN Wins:      {cnn_wins}")
print(f"Baseline Wins: {baseline_wins}")
print(f"Errors/Draws:  {errors}")
if total_valid > 0:
    print(f"CNN Win Rate:  {cnn_wins / total_valid * 100:.1f}%")
print("===============================================")
