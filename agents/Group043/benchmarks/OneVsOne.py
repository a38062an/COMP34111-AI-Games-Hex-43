import argparse
import sys
import os
import importlib
import csv
import time

# ---------------------------------------------------------------------
# PATH SETUP
# ---------------------------------------------------------------------
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
if project_root not in sys.path:
    sys.path.append(project_root)
# ---------------------------------------------------------------------

from src.Colour import Colour
from src.GameOptimised import Game
from src.Player import Player

def get_class(class_path):
    module_path, class_name = class_path.rsplit(".", 1)
    module = importlib.import_module(module_path)
    return getattr(module, class_name)

def run_tournament(num_games, agent1_path, agent2_path, time_limit_ms=None, verbose=False, csv_file=None, name1_override=None, name2_override=None):
    # Dynamically load the agent classes
    Agent1Class = get_class(agent1_path)
    Agent2Class = get_class(agent2_path)

    # Use short names for display (e.g. "BaselineAgent" from "agents.Group43.Agents.BaselineAgent")
    name1 = name1_override if name1_override else agent1_path.split(".")[-1]
    name2 = name2_override if name2_override else agent2_path.split(".")[-1]

    # If names are identical (Self-Play) and no overrides provided, append indices
    if name1 == name2 and not name1_override and not name2_override:
        name1 += "_1"
        name2 += "_2"

    wins1 = 0
    wins2 = 0

    print(f"Starting Tournament: {name1} vs {name2}")
    print(f"Total Games: {num_games}")
    if time_limit_ms:
        print(f"Time Limit: {time_limit_ms}ms")
    if csv_file:
         print(f"Logging results to: {csv_file}")
         
         # Initialize CSV header if file doesn't exist
         file_exists = os.path.isfile(csv_file)
         with open(csv_file, 'a', newline='') as f:
             writer = csv.writer(f)
             if not file_exists:
                 writer.writerow(["GameNum", "WinnerName", "Agent1", "Agent2", "Agent1Time", "Agent2Time", "TotalTurns", "Agent1TimePerTurn", "Agent2TimePerTurn", "WinMethod", "TimeLimit"])

    print("---------------------------------------------------")

    for i in range(1, num_games + 1):
        # Swap sides every game to ensure fairness
        agent1_is_red = (i % 2 != 0)

        # Build kwargs if time_limit is provided
        kwargs = {}
        if time_limit_ms is not None:
            kwargs["time_limit_ms"] = time_limit_ms

        if agent1_is_red:
            p1_agent = Agent1Class(Colour.RED, **kwargs)
            p1_name  = f"{name1} (Red)"
            p2_agent = Agent2Class(Colour.BLUE, **kwargs)
            p2_name  = f"{name2} (Blue)"
        else:
            p1_agent = Agent2Class(Colour.RED, **kwargs)
            p1_name  = f"{name2} (Red)"
            p2_agent = Agent1Class(Colour.BLUE, **kwargs)
            p2_name  = f"{name1} (Blue)"

        # Initialize Game
        # FIX: Use os.devnull instead of None to safely disable logging
        game = Game(
            player1=Player(p1_name, p1_agent),
            player2=Player(p2_name, p2_agent),
            board_size=11,
            verbose=verbose,
            logDest=os.devnull
        )

        # Run Game
        if verbose:
            print(f"Game {i}: {p1_name} vs {p2_name} ...")

        try:
            result = game.run()
            
            winner_name = "Unknown"
            win_method = "Unknown"
            
            # For logging
            stats = result if isinstance(result, dict) else {}
            
            # Default values if stats missing
            t1_time = stats.get('player1_move_time', 0.0)
            t2_time = stats.get('player2_move_time', 0.0)
            t1_turns = stats.get('player_1_turn', 0)
            t2_turns = stats.get('player_2_turn', 0)
            total_turns = stats.get('total_turns', 0)
            win_method = stats.get('win_method', 'Normal')

            if isinstance(result, dict):
                # If result is a dict (from format_result), parse winner name
                winner_name_str = result.get("winner")
                # Map back to agent
                if winner_name_str == p1_name:
                    winner_name = name1 if agent1_is_red else name2
                    if agent1_is_red: wins1 += 1
                    else: wins2 += 1
                elif winner_name_str == p2_name:
                    winner_name = name2 if agent1_is_red else name1
                    if agent1_is_red: wins2 += 1
                    else: wins1 += 1
                else: 
                    winner_name = "Draw"
            else:
                # Fallback
                pass
            
            # CSV Logging
            if csv_file:
                 with open(csv_file, 'a', newline='') as f:
                     writer = csv.writer(f)
                     # Map times correctly to Agent 1 / Agent 2 (regardless of Color)
                     # If agent1_is_red: Agent1=Red=player1, Agent2=Blue=player2
                     # If !agent1_is_red: Agent1=Blue=player2, Agent2=Red=player1
                     
                     a1_t = t1_time if agent1_is_red else t2_time
                     a2_t = t2_time if agent1_is_red else t1_time
                     a1_turns = t1_turns if agent1_is_red else t2_turns
                     a2_turns = t2_turns if agent1_is_red else t1_turns
                     
                     a1_tpt = a1_t / a1_turns if a1_turns > 0 else 0
                     a2_tpt = a2_t / a2_turns if a2_turns > 0 else 0
                     
                     time_limit_s = time_limit_ms / 1000.0 if time_limit_ms else 0
                     
                     writer.writerow([i, winner_name, name1, name2, f"{a1_t:.3f}", f"{a2_t:.3f}", total_turns, f"{a1_tpt:.3f}", f"{a2_tpt:.3f}", win_method, f"{time_limit_s:.3f}"])

        except Exception as e:
            print(f"Game crashed: {e}")
            if csv_file:
                 with open(csv_file, 'a', newline='') as f:
                     writer = csv.writer(f)
                     writer.writerow([i, "CRASH", name1, name2, 0, 0, 0, str(e), time_limit_ms])
            continue

            if verbose:
                print(f"Game {i} Winner: {winner_name} ({win_method})")
                print(f"  {name1}: {a1_t:.3f}s")
                print(f"  {name2}: {a2_t:.3f}s")
                print(f"  Total Turns: {total_turns}")
            else:
                print(f"Game {i}: {winner_name} ({a1_t:.1f}s vs {a2_t:.1f}s)")

    print("\n---------------------------------------------------")
    print("TOURNAMENT RESULTS")
    print("---------------------------------------------------")
    print(f"Total Games: {num_games}")
    print(f"{name1}: {wins1} ({(wins1/num_games)*100:.2f}%)")
    print(f"{name2}: {wins2} ({(wins2/num_games)*100:.2f}%)")
    print("---------------------------------------------------")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a tournament between two agents.")
    parser.add_argument("games", type=int, nargs='?', default=1, help="Number of games to play")
    parser.add_argument("--games", type=int, dest="games_opt", help="Number of games (optional flag)")
    parser.add_argument("--agent1", type=str, default="agents.Group43.Agents.BaselineAgent", help="Class path for Agent 1")
    parser.add_argument("--agent2", type=str, default="agents.Group43.Agents.ExperimentalAgent", help="Class path for Agent 2")
    parser.add_argument("--name1", type=str, default=None, help="Display name for Agent 1")
    parser.add_argument("--name2", type=str, default=None, help="Display name for Agent 2")
    parser.add_argument("--time_limit_ms", type=int, default=None, help="Time limit per player in ms")
    parser.add_argument("--csv_file", type=str, default=None, help="Path to output CSV file")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show move-by-move output")

    args = parser.parse_args()
    
    # Handle games argument flexibility
    games_count = args.games_opt if args.games_opt else args.games

    run_tournament(games_count, args.agent1, args.agent2, args.time_limit_ms, args.verbose, args.csv_file, args.name1, args.name2)
