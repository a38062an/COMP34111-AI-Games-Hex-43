import os
import sys
import csv
import importlib
import concurrent.futures

# Configure Path to find 'src' module
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

from src.Game import Game
from src.Player import Player
from src.Colour import Colour

# Constants
RESULTS_DIR = os.path.join(project_root, "agents/Group43/results")
PLOTS_DIR = os.path.join(project_root, "agents/Group43/plots")

def ensure_dirs():
    """Ensure output directories exist."""
    os.makedirs(RESULTS_DIR, exist_ok=True)
    os.makedirs(PLOTS_DIR, exist_ok=True)

def run_game_internal(test_time, baseline_time, swap_sides):
    """
    Runs a single game.
    If swap_sides is False: TestAgent (Red) vs Baseline (Blue)
    If swap_sides is True:  Baseline (Red) vs TestAgent (Blue)
    """
    import agents.Group43.Agents as Agents
    importlib.reload(Agents) 
    
    if not swap_sides:
        # Test is Red
        p1_agent = Agents.BaselineAgent(Colour.RED, time_limit_ms=test_time)
        p2_agent = Agents.BaselineAgent(Colour.BLUE, time_limit_ms=baseline_time)
        p1_name = "AggressiveAgent"
        p2_name = "BaselineAgent"
    else:
        # Test is Blue
        p1_agent = Agents.BaselineAgent(Colour.RED, time_limit_ms=baseline_time)
        p2_agent = Agents.BaselineAgent(Colour.BLUE, time_limit_ms=test_time)
        p1_name = "BaselineAgent"
        p2_name = "AggressiveAgent"
    
    # Initialize Game
    game = Game(
        player1=Player(p1_name, p1_agent),
        player2=Player(p2_name, p2_agent),
        board_size=11,
        logDest=os.devnull,
        verbose=False
    )
    
    return game.run()

def get_existing_progress(csv_file):
    """Reads the CSV to find out how many games have been played for each config."""
    if not os.path.exists(csv_file):
        return {}
    
    counts = {}
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                try:
                    t_config = int(float(row['TestTimeConfig'])) 
                    counts[t_config] = counts.get(t_config, 0) + 1
                except (ValueError, KeyError):
                    continue
    except Exception as e:
        print(f"Warning: Could not read existing progress: {e}")
        return {}
    return counts

def main():
    ensure_dirs()
    
    # Experiment Configuration
    baseline_time = 150000 
    test_times = [20000, 30000, 50000, 70000, 100000]
    games_per_config = 30 
    max_workers = 4 
    
    csv_file = os.path.join(RESULTS_DIR, "diminishing_returns.csv")
    print(f"Starting High Aggression Benchmark (Fair Mode)... Results -> {csv_file}")
    
    # Check for existing progress
    existing_counts = get_existing_progress(csv_file)
    if existing_counts:
        print(f"Resuming from existing data: {existing_counts}")
    else:
        # Initialize CSV with headers
        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["TestTimeConfig", "GameNum", "Winner", "TestAgentTime", "BaselineAgentTime", "Score", "TotalTurns", "P1Turns"])

    # Run games in parallel
    with concurrent.futures.ProcessPoolExecutor(max_workers=max_workers) as executor:
        for t_limit in test_times:
            done_so_far = existing_counts.get(t_limit, 0)
            
            if done_so_far >= games_per_config:
                print(f"Skipping {t_limit}ms (Completed {done_so_far}/{games_per_config})")
                continue

            remaining = games_per_config - done_so_far
            print(f"\n--- Testing Time Limit: {t_limit/1000}s vs Baseline (Scheduling {remaining} games) ---")
            
            # Map future to swap status
            future_to_swap = {}
            
            for i in range(remaining):
                abs_game_num = done_so_far + i + 1
                # Swap sides on even games (2, 4, 6...)
                swap = (abs_game_num % 2 == 0)
                
                f = executor.submit(run_game_internal, t_limit, baseline_time, swap)
                future_to_swap[f] = (swap, abs_game_num)
            
            completed_in_this_run = 0
            for future in concurrent.futures.as_completed(future_to_swap):
                swap, current_game_num = future_to_swap[future]
                completed_in_this_run += 1
                
                try:
                    stats = future.result()
                    
                    winner_name = stats["winner"]
                    p1_time = float(stats["player1_move_time"])
                    p2_time = float(stats["player2_move_time"])
                    
                    # Map times based on swap
                    if not swap:
                        # Test was Red (P1)
                        test_agent_time = p1_time
                        baseline_agent_time = p2_time
                    else:
                        # Test was Blue (P2)
                        test_agent_time = p2_time
                        baseline_agent_time = p1_time

                    # 1 = AggressiveAgent, 2 = Baseline
                    is_test_winner = (winner_name == "AggressiveAgent")
                    winner_val = 1 if is_test_winner else 2
                    
                    # Score calculation
                    speed_score = max(0.0, 1.0 - (test_agent_time / 300.0))
                    win_score = 1.0 if is_test_winner else 0.0
                    total_score = (0.75 * win_score) + (0.25 * speed_score)
                    
                    total_turns = stats["total_turns"]
                    p1_turns = stats["player1_turns"] # Correct key for turn count

                    with open(csv_file, 'a', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow([t_limit, current_game_num, winner_val, test_agent_time, baseline_agent_time, total_score, total_turns, p1_turns])
                    
                    print(f"[{current_game_num}/{games_per_config}] Win: {winner_name} (Swap={swap}), TestTime: {test_agent_time:.3f}s, Score: {total_score:.3f}")
                    
                except Exception as e:
                    print(f"Game failed: {e}")

if __name__ == "__main__":
    main()
