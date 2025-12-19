import argparse
import sys
import os

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
from agents.Group43.Agents import BaselineAgent, ExperimentalAgent

def run_tournament(num_games, verbose=False):
    baseline_wins = 0
    experimental_wins = 0

    print("Starting Tournament: Baseline vs Experimental")
    print(f"Total Games: {num_games}")
    print("---------------------------------------------------")

    for i in range(1, num_games + 1):
        # Swap sides every game to ensure fairness
        baseline_is_red = (i % 2 != 0)

        if baseline_is_red:
            p1_agent = BaselineAgent(Colour.RED)
            p1_name  = "Baseline (Red)"
            p2_agent = ExperimentalAgent(Colour.BLUE)
            p2_name  = "Experimental (Blue)"
        else:
            p1_agent = ExperimentalAgent(Colour.RED)
            p1_name  = "Experimental (Red)"
            p2_agent = BaselineAgent(Colour.BLUE)
            p2_name  = "Baseline (Blue)"

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

        winner_colour = game.run()

        # Determine who won
        if winner_colour == Colour.RED:
            if baseline_is_red:
                baseline_wins += 1
                winner_name = "Baseline"
            else:
                experimental_wins += 1
                winner_name = "Experimental"
        else:
            if not baseline_is_red:
                baseline_wins += 1
                winner_name = "Baseline"
            else:
                experimental_wins += 1
                winner_name = "Experimental"

        if verbose:
            print(f"Game {i} Winner: {winner_name}")
        else:
            print(".", end="", flush=True)

    print("\n---------------------------------------------------")
    print("TOURNAMENT RESULTS")
    print("---------------------------------------------------")
    print(f"Total Games: {num_games}")
    print(f"Baseline Wins:     {baseline_wins} ({(baseline_wins/num_games)*100:.2f}%)")
    print(f"Experimental Wins: {experimental_wins} ({(experimental_wins/num_games)*100:.2f}%)")
    print("---------------------------------------------------")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a tournament between Baseline and Experimental agents.")
    parser.add_argument("games", type=int, help="Number of games to play")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show move-by-move output")

    args = parser.parse_args()

    run_tournament(args.games, args.verbose)
