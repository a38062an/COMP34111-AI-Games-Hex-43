# Group 43 Hex Agent

A high-performance Hybrid Hex Agent implemented in C++. The architecture is modeled after the world-champion program **MoHex 2.0**, integrating Monte Carlo Tree Search (MCTS) with RAVE, tactical pruning, and neural network policy guidance.

## 1. Quick Start

### Prerequisites

- GCC Compiler (C++17 support)
- Python 3.x
- Make

### Compilation

The agent is written in C++ for performance and must be compiled before use. Running Hex.py will do this compilcaition automatically, but it can also be done explicitly.

```bash
# Compile the agent binary
make
```

This produces the `CppAgent` binary in `bin/`.

### Running the Agent

The agent communicates via standard I/O and is designed to run via the `Hex.py` tournament runner.

1.  **Register the Agent:**
    Ensure `agents/Group43/cmd.txt` exists and contains:
    ```plaintext
    agents.Group43.Group43Agent Group43Agent
    ```
2.  **Run against a Random Bot:**
    ```bash
    python3 Hex.py -p1 "agents.Group43.Group43Agent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
    ```
3.  **Run Self-Play:**
    ```bash
    python3 Hex.py -p1 "agents.Group43.Group43Agent Group43Agent" -p2 "agents.Group43.Group43Agent Group43Agent" -b 11 -v
    ```

## 2. System Architecture

The system follows a modular Controller-Brain-Data architecture designed to support the Hybrid MCTS approach outlined in our project proforma.

### Core Components

| Component          | File              | Role                                                                                                            |
| :----------------- | :---------------- | :-------------------------------------------------------------------------------------------------------------- |
| **Infrastructure** | `Group43Agent.py` | Python wrapper. Spawns the C++ subprocess and translates the engine's text protocol.                            |
| **Entry Point**    | `CppAgent.cpp`    | Parses command line arguments and instantiates the HexAgent.                                                    |
| **Controller**     | `HexAgent.cpp`    | Manages the game loop, protocol parsing, time management, and the Pie Rule (Swap).                              |
| **The Brain**      | `MCTS.cpp`        | The core search engine. Currently implements MCTS + RAVE. Designed to integrate with Neural Policy in Phase 2.  |
| **Data Layer**     | `Bitboard.h`      | An efficient 11x11 board representation using `std::bitset` for $O(1)$ operations and fast connectivity checks. |
| **Memory**         | `Node.h`          | Custom memory-pooled tree nodes for high-performance expansion.                                                 |

### Data Flow: "The Life of a Move"

1.  **Input:** The Python engine sends a protocol string (e.g., `CHANGE;5,5;...`) to stdin.
2.  **Update:** `HexAgent` parses the string and updates the persistent Master Bitboard.
3.  **Instantiation:** A new MCTS instance is created, initialized with a copy of the Master Bitboard.
4.  **Search:** `MCTS::runSearch` executes the 4-phase loop (Selection, Expansion, Simulation, Backpropagation).
    - **Current Optimization:** Uses RAVE (Rapid Action Value Estimation) to accelerate learning.
    - **Planned Integration:** Will query CNN Policy for move priors and H-Search for tactical pruning.
5.  **Output:** The best move is calculated, returned to `HexAgent`, and printed to stdout.

## 3. Project Roadmap & Division of Labor

Our development follows the Proforma strategy, dividing the Hybrid MoHex architecture into distinct streams.

1.  **✅ Phase 1: Corwhe Engine (Anthony)**

    - **Status:** Completed
    - **Feature:** High-performance MCTS loop.
    - **Feature:** RAVE (Rapid Action Value Estimation) algorithm implementation.
    - **Optimization:** Tree structure optimization (Memory Pools, Transposition Tables).

2.  **🚧 Phase 2: Tactical Solver (Joshua)**

    - **Status:** In Progress
    - **Feature:** H-Search algorithm for virtual connections.
    - **Feature:** Deterministic pruning of solved sub-problems (e.g., Bridges, forced wins).

3.  **🚧 Phase 3: Simulation Strategy (Alex)**

    - **Status:** In Progress
    - **Feature:** Pattern-based playouts to replace random rollouts.
    - **Goal:** Fast lookup of 3x3 local patterns to guide simulations.

4.  **🔮 Phase 4: Neural Policy (Alexios)**
    - **Status:** Planned
    - **Feature:** Deep Convolutional Neural Network (CNN) trained on expert games.
    - **Goal:** Provide move priors to reduce the effective branching factor.

## 4. Directory Structure

```plaintext
agents/Group43/
├── cmd.txt                 # Entry point definition for tournament runner
├── Group43Agent.py         # Python Wrapper
├── README.md               # This file
├── Makefile                # Build script
├── src/                    # C++ Source Code
│   ├── CppAgent.cpp        # Main Entry Point
│   ├── HexAgent.cpp        # Controller Logic
│   ├── MCTS.cpp            # Search Logic (RAVE + MCTS)
│   ├── Bitboard.h          # Board Representation
│   └── Node.h              # Tree Node Structure (Memory Pool)
├── bin/                    # Compiled C++ binaries
├── results/                # Results of our experiments
│   ├── logs                # Benchmarking Logs
│   └── plots               # Plots for evaluation
└── experiments/            # Benchmarking Tools
    ├── run_experiments.sh  # Bash script to run experiments
    ├── plot_experiments.py # Python script to create plots
    └── Experiment.cpp      # Performance testing runner
```
