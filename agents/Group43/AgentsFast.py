
from agents.Group43.Agents import Group43AgentBase
from src.Colour import Colour

class FastCNNAgent(Group43AgentBase):
    def __init__(self, colour: Colour):
        super().__init__(colour, binary_name="bin/CppAgent", extra_args=["--time", "1000"])

class FastBaselineAgent(Group43AgentBase):
    def __init__(self, colour: Colour):
        super().__init__(colour, binary_name="bin/CppAgent", extra_args=["--no-cnn", "--time", "1000"])
