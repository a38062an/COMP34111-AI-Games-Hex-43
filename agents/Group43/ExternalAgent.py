import os
from subprocess import PIPE, Popen
from src.AgentBase import AgentBase
from src.Colour import Colour
from agents.DefaultAgents.ExternalAgent import ExternalAgent

class Group43Agent(ExternalAgent):
    """
    Extension of the Default ExternalAgent to run a C++ binary instead of Java.
    Inherits protocol handling (make_move) and deepcopy logic from ExternalAgent.
    """
    
    def __init__(self, colour: Colour):
        # Initialize AgentBase directly to skip ExternalAgent's __init__ 
        # (which would try to spawn the Java agent)
        AgentBase.__init__(self, colour)
        
        # Path to the compiled C++ executable
        base_dir = os.path.dirname(os.path.abspath(__file__))
        executable_path = os.path.join(base_dir, "src", "CppAgent")
        
        # Check if executable exists, if not, try to compile it
        if not os.path.exists(executable_path):
            print(f"Warning: C++ executable not found at {executable_path}. Trying to compile...")
            import subprocess
            subprocess.run(["make"], cwd=os.path.join(base_dir, "src"), check=True)

        self.agent_process = Popen(
            [
                executable_path,
                colour.get_char(),
                "11",
            ],
            stdout=PIPE,
            stdin=PIPE,
            text=True,
            bufsize=1 # Line buffered
        )

    # make_move and __deepcopy__ are inherited from ExternalAgent
