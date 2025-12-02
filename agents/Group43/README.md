# Group 43 Hex Agent

This directory contains the C++ Hex agent for Group 43.

## 1. Compilation

The agent is written in C++ and must be compiled before running.

**To compile:**
```bash
make -C src
```
This will produce the `CppAgent` executable in the `src/` directory.

> **Note:** The Python wrapper (`ExternalAgent.py`) will attempt to compile the code automatically if the executable is missing, but it is recommended to compile manually after making changes.

## 2. Running the Agent

You can run the agent using the `Hex.py` game engine from the root of the project.

### Against NaiveAgent (Random Bot)
This is the best way to verify that the agent is working correctly.

```bash
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
```

### Against Itself (Self-Play)
To see how the agent plays against itself:

```bash
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.Group43.ExternalAgent Group43Agent" -b 11 -v
```

### Command Arguments
*   `-p1`: Specifies Player 1 (Red).
*   `-p2`: Specifies Player 2 (Blue).
*   `-b 11`: Sets the board size to 11x11.
*   `-v`: Enables verbose output (shows the board and moves in the terminal).

## 3. Project Structure

*   `ExternalAgent.py`: The Python wrapper that the tournament runner interacts with. It launches the C++ executable.
*   `src/`: Contains the C++ source code.
    *   `CppAgent.cpp`: Main entry point.
    *   `HexAgent.cpp`: Agent logic and protocol handling.
    *   `MCTS.cpp`: Monte Carlo Tree Search implementation.
    *   `Bitboard.h`: Efficient board representation.
*   `PROTOCOL.md`: Documentation of the communication protocol between Python and C++.

## 4. Running in Docker

To ensure the agent runs in the competition environment, you should test it inside Docker.

### 1. Build the Image
From the root of the project:
```bash
docker build -t hex-agent .
```

### 2. Run the Container
Mount your current directory so you can edit files on your host and run them in Docker:
```bash
docker run -it --rm -v $(pwd):/home/hex hex-agent
```

### 3. Recompile inside Docker (CRITICAL)
Binaries compiled on macOS (ARM64/x86) will **NOT** run on Linux. You must recompile inside the container:
```bash
# Inside the container:
make -C agents/Group43/src clean && make -C agents/Group43/src
```

### 4. Run the Agent
Now you can run the agent as usual:
```bash
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
```
