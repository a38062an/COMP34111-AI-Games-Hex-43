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
    # Defaults
    # Use absolute path relative to this file to allow running from anywhere
    MAKEFILE_DIR = os.path.dirname(os.path.abspath(__file__))

    def __init__(self, colour: Colour, binary_name: str, extra_args: list = None):
        super().__init__(colour)
        
        # binary_name passed as "bin/CppAgent", but MAKEFILE_DIR is .../Group43
        # So we want .../Group43/bin/CppAgent.
        # However, binary_name might already include 'bin/' if passed from subclass.
        # Let's clean it up.
        
        self.executable_path = os.path.join(self.MAKEFILE_DIR, binary_name)
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

class BaselineAgent(Group43AgentBase):
    """
    The Stable/Master version. 
    Uses 'bin/CppAgent' (Compiled via 'make' on main branch).
    """
    def __init__(self, colour: Colour, time_limit_ms: int = 300000):
        super().__init__(
            colour, 
            binary_name="bin/CppAgent",
            extra_args=["11", str(time_limit_ms)]
        )

class ExperimentalAgent(Group43AgentBase):
    """
    The Feature/Dev version.
    Uses 'bin/DevAgent' (Compiled via 'make dev' on feature branch).
    """
    def __init__(self, colour: Colour, time_limit_ms: int = 300000):
        super().__init__(
            colour,
            binary_name="bin/DevAgent",
            extra_args=["11", str(time_limit_ms)]
        )
