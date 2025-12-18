import os
import subprocess
import sys
from copy import deepcopy
from subprocess import PIPE, Popen

from src.AgentBase import AgentBase
from src.Board import Board
from src.Colour import Colour
from src.Move import Move


class Group43Agent(AgentBase):
    """
    A standalone wrapper for the Group43 C++ Hex Agent.
    Inherits from AgentBase and manages its own C++ subprocess.
    """

    # --- Constants for easy configuration ---
    # Resolve paths relative to this file to work from any CWD
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    EXECUTABLE_PATH = os.path.join(BASE_DIR, "bin", "CppAgent")
    MAKEFILE_DIR = BASE_DIR
    BOARD_SIZE = "11"

    def __init__(self, colour: Colour):
        super().__init__(colour)

        # 1. Compile if necessary
        if not os.path.exists(self.EXECUTABLE_PATH):
            print(f"[Group43] Executable not found at {self.EXECUTABLE_PATH}. Compiling...", file=sys.stderr)
            try:
                subprocess.run(
                    ["make"],
                    cwd=self.MAKEFILE_DIR,
                    check=True,
                    capture_output=True,
                    text=True
                )
                print("[Group43] Compilation successful.", file=sys.stderr)
            except subprocess.CalledProcessError as e:
                print(f"[Group43] Compilation FAILED. Return Code: {e.returncode}", file=sys.stderr)
                print(f"STDOUT:\n{e.stdout}", file=sys.stderr)
                print(f"STDERR:\n{e.stderr}", file=sys.stderr)
                # If compilation fails, we can't play.
                sys.exit(1)

        # 2. Launch the C++ Process
        cmd_args = [
            self.EXECUTABLE_PATH,
            colour.get_char(),
            self.BOARD_SIZE,
        ]

        # Check for environment configuration
        # G43_TIME_LIMIT: int (ms)
        # Purely for bench marking DEBUG
        time_limit = os.environ.get("G43_TIME_LIMIT")

        if time_limit:
            cmd_args.append(time_limit)

        try:
            self.agent_process = Popen(
                cmd_args,
                stdout=PIPE,
                stdin=PIPE,
                text=True,
                bufsize=1  # Line buffered for smoother IPC
            )
        except Exception as e:
            print(f"[Group43] Failed to launch subprocess: {e}", file=sys.stderr)
            sys.exit(1)

    def make_move(self, turn: int, board: Board, opp_move: Move | None) -> Move:
        """
        Translates the Python game state into the string protocol 
        expected by the C++ agent.
        """

        # 1. Convert Board to String
        # (Same logic as ExternalAgent, but kept self-contained here)
        board_strings = []
        for row in board.tiles:
            row_string = ""
            for tile in row:
                if tile.colour is None:
                    row_string += "0"
                else:
                    row_string += tile.colour.get_char()
            board_strings.append(row_string)
        board_string = ",".join(board_strings)

        # 2. Construct the Command
        # Protocol: COMMAND;OPP_MOVE;BOARD;TURN;
        if opp_move is None:
            # First move of the game (if we are Red)
            command = f"START;;{board_string};{turn};"
        elif opp_move.is_swap():
            # Opponent performed the Pie Rule swap
            command = f"SWAP;;{board_string};{turn};"
        else:
            # Standard opponent move
            command = f"CHANGE;{opp_move.x},{opp_move.y};{board_string};{turn};"

        # 3. Send to C++
        try:
            self.agent_process.stdin.write(command + "\n")
            self.agent_process.stdin.flush()

            # 4. Read Response
            response = self.agent_process.stdout.readline().rstrip()

            if not response:
                raise ValueError("Received empty response from C++ agent.")

            # Parse "x,y"
            x, y = response.split(",")
            return Move(int(x), int(y))

        except (BrokenPipeError, ValueError) as e:
            print(f"[Group43] Communication error: {e}", file=sys.stderr)
            # Return a resign/invalid move or re-raise
            return Move(-100, -100) 

    def __deepcopy__(self, memo):
        """
        Crucial: Prevents the subprocess from being copied during
        Game engine security checks.
        """
        cls = self.__class__
        result = cls.__new__(cls)
        memo[id(self)] = result
        result.__dict__ = {
            k: deepcopy(v, memo)
            for k, v in self.__dict__.items()
            if k != "agent_process"
        }
        result.agent_process = None
        return result
