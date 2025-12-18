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

def run_game_internal(time_limit_ms_p1, time_limit_ms_p2):
    """
    Runs a single game using the internal game engine.
    """
    
    # Configure Agent 1 (Red)
    os.environ["G43_TIME_LIMIT"] = str(time_limit_ms_p1)
    
    # Reload module
    import agents.Group43.Group43Agent as agent_module
    importlib.reload(agent_module)
    AgentClass = getattr(agent_module, "Group43Agent")
    agent1 = AgentClass(Colour.RED)
    
    # Configure Agent 2 (Blue)
    os.environ["G43_TIME_LIMIT"] = str(time_limit_ms_p2)
    
    agent2 = AgentClass(Colour.BLUE)
    
    # Initialize Game
    game = Game(
        player1=Player("AggressiveAgent", agent1),
        player2=Player("BaselineAgent", agent2),
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
                    t_config = int(row['TestTimeConfig'])
                    counts[t_config] = counts.get(t_config, 0) + 1
                except (ValueError, KeyError):
                    continue
    except Exception as e:
        print(f"Warning: Could not read existing progress: {e}")
        return {}
    return counts

def main():
    ensure_dirs()
    
    # Experiment Configuration for Diminishing Results
    baseline_time = 150000 
    test_times = [30000, 60000, 120000, 180000, 240000, 300000]
    games_per_config = 30 
    max_workers = 4 
    
    csv_file = os.path.join(RESULTS_DIR, "diminishing_returns.csv")
    print(f"Starting High Aggression Benchmark... Results -> {csv_file}")
    
    # Check for existing progress
    existing_counts = get_existing_progress(csv_file)
    if existing_counts:
        print(f"Resuming from existing data: {existing_counts}")
        mode = 'a'
    else:
        mode = 'w'
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
            
            futures = []
            for i in range(remaining):
                futures.append(
                    executor.submit(run_game_internal, t_limit, baseline_time)
                )
            
            completed_in_this_run = 0
            for future in concurrent.futures.as_completed(futures):
                completed_in_this_run += 1
                current_game_num = done_so_far + completed_in_this_run
                
                try:
                    stats = future.result()
                    
                    winner_name = stats["winner"]
                    t1_s = float(stats["player1_move_time"])
                    t2_s = float(stats["player2_move_time"])
                    
                    # 1 = AggressiveAgent, 2 = Baseline
                    is_test_winner = (winner_name == "AggressiveAgent")
                    winner_val = 1 if is_test_winner else 2
                    
                    speed_score = max(0.0, 1.0 - (t1_s / 300.0))
                    win_score = 1.0 if is_test_winner else 0.0
                    total_score = (0.75 * win_score) + (0.25 * speed_score)
                    
                    total_turns = stats["total_turns"]
                    p1_turns = stats["player1_turns"]

                    with open(csv_file, 'a', newline='') as f:
                        writer = csv.writer(f)
                        writer.writerow([t_limit, current_game_num, winner_val, t1_s, t2_s, total_score, total_turns, p1_turns])
                    
                    print(f"[{current_game_num}/{games_per_config}] Win: {winner_name}, T1: {t1_s}s, Score: {total_score:.3f}")
                    
                except Exception as e:
                    print(f"Game failed: {e}")

if __name__ == "__main__":
    main()
