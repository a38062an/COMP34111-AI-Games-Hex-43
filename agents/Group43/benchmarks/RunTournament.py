import argparse
import sys
import os
import importlib

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

def get_class(kls):
    parts = kls.split('.')
    module = ".".join(parts[:-1])
    m = importlib.import_module(module)
    return getattr(m, parts[-1])

def run_tournament(num_games, agent1_path, agent2_path, verbose=False):
    Agent1Class = get_class(agent1_path)
    Agent2Class = get_class(agent2_path)

    agent1_wins = 0
    agent2_wins = 0
    
    # Extract short names for display
    name1 = agent1_path.split('.')[-1]
    name2 = agent2_path.split('.')[-1]

    print(f"Starting Tournament: {name1} vs {name2}")
    print(f"Total Games: {num_games}")
    print("---------------------------------------------------")

    for i in range(1, num_games + 1):
        # Swap sides every game to ensure fairness
        agent1_is_red = (i % 2 != 0)

        if agent1_is_red:
            p1_agent = Agent1Class(Colour.RED, time_limit_ms=180000)
            p1_name  = f"{name1} (Red)"
            p2_agent = Agent2Class(Colour.BLUE, time_limit_ms=180000)
            p2_name  = f"{name2} (Blue)"
        else:
            p1_agent = Agent2Class(Colour.RED, time_limit_ms=180000)
            p1_name  = f"{name2} (Red)"
            p2_agent = Agent1Class(Colour.BLUE, time_limit_ms=180000)
            p2_name  = f"{name1} (Blue)"

        # Initialize Game
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

        winner_colour = game.run()

        # Determine who won
        if winner_colour == Colour.RED:
            if agent1_is_red:
                agent1_wins += 1
                winner_name = name1
            else:
                agent2_wins += 1
                winner_name = name2
        else:
            if not agent1_is_red:
                agent1_wins += 1
                winner_name = name1
            else:
                agent2_wins += 1
                winner_name = name2

        if verbose:
            print(f"Game {i} Winner: {winner_name}")
        else:
            print(".", end="", flush=True)

    print("\n---------------------------------------------------")
    print("TOURNAMENT RESULTS")
    print("---------------------------------------------------")
    print(f"Total Games: {num_games}")
    print(f"{name1}: {agent1_wins} ({(agent1_wins/num_games)*100:.2f}%)")
    print(f"{name2}: {agent2_wins} ({(agent2_wins/num_games)*100:.2f}%)")
    print("---------------------------------------------------")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a tournament between two agents.")
    parser.add_argument("--games", type=int, default=10, help="Number of games to play")
    parser.add_argument("--agent1", type=str, required=True, help="Classpath for Agent 1 (e.g., agents.Group43.Agents.SwapAgent)")
    parser.add_argument("--agent2", type=str, required=True, help="Classpath for Agent 2 (e.g., agents.Group43.Agents.NoSwapAgent)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show move-by-move output")

    args = parser.parse_args()

    run_tournament(args.games, args.agent1, args.agent2, args.verbose)
