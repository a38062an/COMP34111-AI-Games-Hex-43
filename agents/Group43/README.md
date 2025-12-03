# Group 43 Hex Agent

This directory contains the high-performance C++ Hex agent for Group 43.

## 1. Quick Start

### Compilation
The agent is written in C++ and must be compiled before running.

```bash
make -C src
```
This produces the `CppAgent` executable in the `src/` directory.

### Running the Agent
You can run the agent using the `Hex.py` game engine from the root of the project.

**Against Random Bot:**
```bash
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
```

**Against Itself:**
```bash
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.Group43.ExternalAgent Group43Agent" -b 11 -v
```

---

## 2. Architecture

### Core Components
*   **`HexAgent`**: The "Game Controller". Handles the communication protocol, manages the master board state, and coordinates the search.
*   **`MCTS`**: The "Brain". Implements Monte Carlo Tree Search with RAVE (Rapid Action Value Estimation) to find the best move.
*   **`Bitboard`**: The "Data Structure". An efficient 11x11 board representation using `std::bitset` for O(1) operations.

### Project Structure
*   `ExternalAgent.py`: Python wrapper that launches the C++ executable.
*   `src/`:
    *   `CppAgent.cpp`: Main entry point.
    *   `HexAgent.cpp`: Agent logic and protocol handling.
    *   `MCTS.cpp`: Search engine.
    *   `Bitboard.h`: Board representation.

---

## 3. MCTS Walkthrough

The agent uses **Monte Carlo Tree Search (MCTS)** with **RAVE**. Here is how it works:

### The 4 Phases
1.  **Selection**: Start at the root. Move down the tree picking the "best" child using the **UCT Formula** (balancing wins vs. visits) until we hit a node with unexplored moves.
2.  **Expansion**: Pick one random unexplored move, create a new child node for it, and add it to the tree.
3.  **Simulation (Rollout)**: Play a completely random game from this new child until someone wins.
4.  **Backpropagation**: Walk back up the tree, updating stats (`visits`, `wins`) for every node we passed through.

### RAVE (Rapid Action Value Estimation)
Standard MCTS learns slowly because it only updates nodes it directly visited. RAVE speeds this up with the **"All-Moves-As-First"** heuristic:
*   **Idea**: If a move is good, it's probably good regardless of *when* it's played.
*   **Mechanism**: If we play Move X deep in a random simulation and win, we credit the child node corresponding to Move X as if we had played it immediately.
*   **Result**: The tree learns good moves 10-100x faster.

---

## 4. Performance Optimizations

To ensure the agent plays at a high level within the 5-minute time limit, we implemented several low-level C++ optimizations:



### 1. Optimized Simulation Loop
*   **Problem**: Creating a `vector` of empty spots for every random move is slow.
*   **Solution**: We use a fixed-size stack array `int moves[121]` and a **Swap-Remove** strategy to pick random moves.
*   **Benefit**: Zero heap allocations during the critical simulation phase.

### 2. Fast Random Number Generator
*   **Problem**: `rand()` is slow.
*   **Solution**: Implemented **Xorshift** RNG.
*   **Benefit**: Generates random numbers with just 3 bitwise operations.

### 3. Compact Node Storage
*   **Problem**: Storing move lists as `vector<pair<int,int>>` is memory-heavy.
*   **Solution**: Converted to `vector<int16_t>` storing flat indices (0-120).
*   **Benefit**: 4x memory reduction, better CPU cache usage.

### 4. Bitboards
*   **Problem**: 2D char arrays are slow to copy and check.
*   **Solution**: Two `std::bitset<121>` (Red/Blue).
*   **Benefit**: Instant win checks and board copying.

### 5. Pie Rule (SWAP) Handling
*   **Problem**: The standard MCTS agent doesn't understand that it might switch sides on turn 2.
*   **Solution**: We explicitly handle the `SWAP` command in `HexAgent.cpp`.
*   **Benefit**: If the opponent swaps, the agent correctly inverts its color and plays for the new side.
