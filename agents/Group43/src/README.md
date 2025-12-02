# C++ Hex Agent

This directory contains the source code for the high-performance C++ Hex agent.

> **Usage:** For instructions on how to compile and run the agent, please see the [main README](../README.md).

## Architecture

### 1. `Bitboard.h`

An efficient representation of the 11x11 Hex board using `std::bitset`.

* **Why Bitsets?** A `bitset` is a fixed-size array of bits (0 or 1). It is extremely memory-efficient (121 bits fits in two 64-bit integers) and fast.
* **Two Bitsets**: We use two separate bitsets:
  * `red`: Stores 1 if a tile has a Red piece, 0 otherwise.
  * `blue`: Stores 1 if a tile has a Blue piece, 0 otherwise.
  * **Reason**: This allows us to check for wins or valid moves for a specific player instantly without parsing characters.
* **Set Logic**: When we `set(x, y, 'R')`, we must:
    1. Set the bit in `red` to 1.
    2. Set the bit in `blue` to 0 (to ensure a tile doesn't have both colors).

### 2. `MCTS.cpp` / `MCTS.h`
The Monte Carlo Tree Search engine.

#### The 4 Phases
1.  **Selection**: Start at the root and move down the tree. At each step, pick the "best" child using the **UCT Formula** until we hit a node that isn't fully expanded (has untried moves).
2.  **Expansion**: Pick one random move from the `untriedMoves` list, create a new child node for it, and add it to the tree.
3.  **Simulation (Rollout)**: From this new child, play random moves for both sides until the game ends (Win/Loss). This gives us a result (e.g., Red Wins).
4.  **Backpropagation**: Take the result (Win/Loss) and walk back up the tree to the root, updating `visits` and `wins` for every node along the path.

#### The Math: UCT Formula
Used in the **Selection** phase to balance Exploration vs. Exploitation.

$$ UCT = \frac{w_i}{n_i} + c \sqrt{\frac{\ln N_i}{n_i}} $$

*   $w_i$: Number of wins for this child node.
*   $n_i$: Number of visits to this child node.
*   $N_i$: Number of visits to the **parent** node.
*   $c$: Exploration constant (usually $\sqrt{2} \approx 1.414$).

**How it works:**
*   **Left Term ($\frac{w_i}{n_i}$)**: Exploitation. "This move has a high win rate, let's pick it."
*   **Right Term**: Exploration. "We haven't visited this node much compared to its parent, let's try it."
    *   As $N_i$ (parent visits) grows, the numerator grows.
    *   If $n_i$ (child visits) is small, the whole term is large.
    *   This forces the algorithm to eventually visit ignored nodes.

### 3. `Node.h`
Defines the `Node` structure used by the MCTS engine.
*   Separated into its own file to avoid circular dependencies and improve cleanliness.

### 4. `HexAgent.cpp` / `HexAgent.h`
The main agent class.
*   Handles the communication protocol (parsing commands, printing board).
*   Manages the `Bitboard` and `MCTS` instances.
*   `run()`: The main game loop.

### 5. `CppAgent.cpp`
The entry point (`main` function).
*   Parses command-line arguments (color, board size).
*   Instantiates and runs the `HexAgent`.

## Compilation
Run `make` in this directory to compile the agent.
```bash
make
```
This produces the `CppAgent` executable.
