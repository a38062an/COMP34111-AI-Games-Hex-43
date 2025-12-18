import os
import subprocess
import sys
from copy import deepcopy
from subprocess import PIPE, Popen

from src.AgentBase import AgentBase
from src.Board import Board
from src.Colour import Colour
from src.Move import Move

class Group43AgentBase(AgentBase):
    """
    Base class for the Group43 Agent. 
    Configurable to launch different binaries.
    """

    # Defaults
    MAKEFILE_DIR = "agents/Group43"

    def __init__(self, colour: Colour, binary_name: str, extra_args: list = None):
        super().__init__(colour)

        self.executable_path = f"{self.MAKEFILE_DIR}/{binary_name}"
        self.extra_args = extra_args if extra_args else []

        # Check if binary exists (We assume manual compilation per the requested workflow)
        if not os.path.exists(self.executable_path):
             print(f"[{binary_name}] Error: Executable not found at {self.executable_path}.", file=sys.stderr)
             print(f"[{binary_name}] Please compile it manually before running the tournament.", file=sys.stderr)
             sys.exit(1)

        # Command structure: ./Executable <Colour> [ExtraArgs...]
        command = [self.executable_path, colour.get_char()] + self.extra_args

        try:
            self.agent_process = Popen(
                command,
                stdout=PIPE,
                stdin=PIPE,
                text=True,
                bufsize=1
            )
        except Exception as e:
            print(f"[{binary_name}] Failed to launch subprocess: {e}", file=sys.stderr)
            sys.exit(1)

    def make_move(self, turn: int, board: Board, opp_move: Move | None) -> Move:
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

        if opp_move is None:
            command = f"START;;{board_string};{turn};"
        elif opp_move.is_swap():
            command = f"SWAP;;{board_string};{turn};"
        else:
            command = f"CHANGE;{opp_move.x},{opp_move.y};{board_string};{turn};"

        try:
            self.agent_process.stdin.write(command + "\n")
            self.agent_process.stdin.flush()
            response = self.agent_process.stdout.readline().rstrip()
            if not response:
                raise ValueError("Received empty response.")
            x, y = response.split(",")
            return Move(int(x), int(y))
        except Exception as e:
            print(f"[Group43] Error: {e}", file=sys.stderr)
            return Move(-100, -100)

    def __deepcopy__(self, memo):
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


# --- AGENT DEFINITIONS ---

class Group43Agent(Group43AgentBase):
    """
    The Main/Tournament version. 
    Current Config: BASELINE MODE (RAVE + UCT) for Maximum Performance.
    (CNN interactions are disabled for the competition).
    """
    def __init__(self, colour: Colour):
        super().__init__(
            colour, 
            binary_name="bin/CppAgent",
            extra_args=["--no-cnn", "--time", "5000"]
        )

class Group43Experimental(Group43AgentBase):
    """
    The Experimental CNN version.
    Uses Neural Network for search guidance.
    """
    def __init__(self, colour: Colour):
        super().__init__(
            colour, 
            binary_name="bin/CppAgent",
            extra_args=["--time", "5000"]
        )

class Group43Baseline(Group43AgentBase):
    """
    The Baseline version.
    """
    def __init__(self, colour: Colour):
        super().__init__(
            colour,
            binary_name="bin/CppAgent",
            extra_args=["--no-cnn", "--time", "5000"]
        )
