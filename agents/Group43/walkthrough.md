# Code Walkthrough: The Life of a Move

This document traces the execution flow of the agent, explaining exactly what functions call what, and how the "Brain" (MCTS) is created and destroyed every turn.

## 1. The Entry Point (`HexAgent::run`)

The agent sits in an infinite loop inside `HexAgent::run()`, waiting for commands from the game engine.

```cpp
// HexAgent.cpp
void HexAgent::run() {
    while (getline(cin, line)) {
        // 1. PARSE: Read command string (e.g., "CHANGE;5,5;...board...")
        // 2. UPDATE: parseBoard(boardString) updates the 'bitboard' member.
        // 3. DECIDE: Call makeMove()
        Point p = makeMove();
        // 4. OUTPUT: Print "row,col"
    }
}
```

*   **State**: `HexAgent` holds the **Master Board** (`bitboard`). This persists across the entire game.

## 2. Making a Decision (`HexAgent::makeMove`)

This is where the "Brain" is born.

```cpp
// HexAgent.cpp
Point HexAgent::makeMove() {
    // 1. CREATE BRAIN: Instantiate MCTS
    // We pass the CURRENT state of the Master Board.
    MCTS mcts(bitboard, myColour); 
    
    // 2. THINK: Run the search for 5 seconds
    pair<int, int> bestMove = mcts.runSearch(5000);
    
    // 3. RETURN: Pass the result back
    return {bestMove.first, bestMove.second};
    
    // 4. DESTROY BRAIN: 'mcts' goes out of scope here.
    // The entire Tree (millions of nodes) is deleted instantly.
}
```

*   **Key Concept**: We create a **NEW Brain** every single turn.
    *   **Why?** The game state has changed (opponent moved). The old tree is mostly invalid/stale. It's faster and safer to build a fresh tree from the new state than to try and "prune" or "re-root" the old tree.
    *   **What it remembers**: Nothing. The new brain starts with a blank slate (just the current board).

## 3. The Thinking Process (`MCTS::runSearch`)

Inside the brain, we run the 4-phase loop thousands of times.

```cpp
// MCTS.cpp
pair<int, int> MCTS::runSearch(int timeLimit) {
    // 1. SETUP: Create Root Node from the board we were given.
    auto root = make_unique<Node>(...);
    
    while (time_left) {
        // 2. COPY: Make a scratchpad copy of the board
        Bitboard simulationBoard = rootBoard;
        
        // 3. SELECT: Walk down the tree
        Node* leaf = select(root, simulationBoard);
        
        // 4. EXPAND: Add one new child
        leaf = expand(leaf, simulationBoard);
        
        // 5. SIMULATE: Play random game on the scratchpad
        Result result = simulate(simulationBoard, ...);
        
        // 6. BACKPROP: Update stats up the tree
        backpropagate(leaf, result);
    }
    
    // 7. DECIDE: Pick child with most visits
    return best_child_move;
}
```

## 4. The Simulation (`MCTS::simulate`)

This is the "inner loop" where 90% of CPU time is spent.

```cpp
// MCTS.cpp
Result MCTS::simulate(Bitboard board, char turn) {
    // 1. SETUP: Create list of all empty spots (on stack)
    int moves[121];
    
    while (!game_over) {
        // 2. PICK: Random move using FastRNG
        // 3. PLAY: Update the 'board' (the scratchpad copy)
        board.set(col, row, turn);
        // 4. SWAP: Switch turns
    }
    return winner;
}
```

## Summary of Data Flow

1.  **Game Engine** sends string -> **HexAgent**.
2.  **HexAgent** updates **Master Board**.
3.  **HexAgent** creates **MCTS** (passes copy of Master Board).
4.  **MCTS** creates **Root Node**.
5.  **MCTS** loop:
    *   Copies Master Board -> **Simulation Board**.
    *   Modifies Simulation Board (Selection/Expansion).
    *   Passes Simulation Board -> **Simulate**.
    *   **Simulate** trashes the board with random moves.
6.  **MCTS** returns best move.
7.  **HexAgent** prints move.
8.  **MCTS** is destroyed (memory freed).

## 5. Special Move Handling

### The Pie Rule (SWAP)
*   **Scenario**: On Turn 2, the second player can choose to swap sides (steal the first player's move).
*   **Handling**:
    *   The engine sends `SWAP`.
    *   `HexAgent` detects this string.
    *   It flips its own color (`myColour = (myColour == 'R') ? 'B' : 'R'`).
    *   It then proceeds to think and play as the *new* color.
    *   This ensures the MCTS always simulates from the correct perspective.
