# Hex AI Technical Design: The "Dual-Mode" Engine

## 1. System Architecture Overview

The system is designed as a **Modular C++ Engine** with a **Pluggable Evaluation Layer**. This allows us to switch between "Neural Mode" (High Intelligence) and "Heuristic Mode" (High Speed) without changing the core search logic.

```mermaid
classDiagram
    class HexAgent {
        +run()
        +handle_protocol()
    }
    class MCTSEngine {
        +search(root_state, time_limit)
        +best_move()
    }
    class Evaluator {
        <<interface>>
        +evaluate(state) -> (policy, value)
    }
    class NNEvaluator {
        +evaluate(state)
        -onnx_session
    }
    class HeuristicEvaluator {
        +evaluate(state)
        -rave_stats
        -vc_solver
    }
    class GameState {
        +bitboard_red
        +bitboard_blue
        +make_move()
        +check_win()
    }

    HexAgent --> MCTSEngine
    MCTSEngine --> GameState
    MCTSEngine --> Evaluator
    Evaluator <|-- NNEvaluator
    Evaluator <|-- HeuristicEvaluator
```

---

## 2. Component Breakdown

### A. The Infrastructure Layer
This layer handles communication with the tournament runner and manages the game loop.

#### 1. Python Wrapper (`MyAgent.py`)
*   **Role:** Process Manager & Protocol Translator.
*   **Key Functions:**
    *   `__init__`: Spawns the C++ subprocess (`./CppAgent`).
    *   `make_move(board)`:
        1.  Converts Python `Board` object to string `R00,0B0,...`.
        2.  Sends `CHANGE;...` or `START;...` to C++ via `stdin`.
        3.  Reads `x,y` from `stdout`.
        4.  Handles crashes (restarts C++ process if it dies).

#### 2. C++ Main Loop (`CppAgent.cpp`)
*   **Role:** Command Processor.
*   **Key Functions:**
    *   `parse_command(string)`: Splits `COMMAND;MOVE;BOARD;TURN` into tokens.
    *   `time_manager()`: Calculates how much time to spend on this move (e.g., "use 9.5s" or "panic mode").
    *   `main()`: Infinite loop waiting for `std::cin`.

---

## 3. The Core Engine Layer (C++)
This is the "Brain" of the bot.

#### 1. Board Representation (`Bitboard.h`)
*   **Data:** Two `__int128` (or `std::bitset<128>`) variables: `red_pieces`, `blue_pieces`.
*   **Key Functions:**
    *   `put(color, x, y)`: Sets a bit.
    *   `is_valid(x, y)`: Checks if bit is 0 in both boards.
    *   `check_win(color)`: Uses bitwise flood-fill to check for connection. **Critical for speed.**
    *   `get_neighbors(x, y)`: Returns a bitmask of neighbors.

#### 2. MCTS Engine (`MCTS.h`)
*   **Data:** Tree of `Node` objects.
*   **Key Functions:**
    *   `select(node)`: Uses UCT formula to descend the tree.
    *   `expand(node)`: Adds children to a leaf node.
    *   `backpropagate(path, result)`: Updates `wins` and `visits` up the tree.
    *   `run_search(root, timeout)`: The main loop. Runs 8 worker threads.

---

## 4. The Pluggable Evaluation Layer (The "Secret Sauce")
This interface allows us to swap brains.

`virtual EvaluationResult evaluate(const GameState& state);`

#### Mode A: Neural Evaluator (`NNEvaluator.cpp`)
*   **Used When:** We want maximum intelligence (AlphaZero style).
*   **Logic:**
    1.  Convert `Bitboard` to `float input[3][11][11]` (Red, Blue, Turn).
    2.  Run `Ort::Session::Run` (ONNX Runtime).
    3.  Return `policy` (vector of probs) and `value` (win prob).
*   **Optimization:** Uses **Int8 Quantization** and **Batching** (collects 32 requests before running).

#### Mode B: Heuristic Evaluator (`HeuristicEvaluator.cpp`)
*   **Used When:** NN is too slow (< 400 evals/s) or for "Panic Mode".
*   **Logic:**
    1.  **Policy:** Use **RAVE** stats. If a move is good in other branches, give it high probability.
    2.  **Value:** Run a **Simulation (Rollout)**.
        *   *Smart Rollout:* Don't play random. If the opponent has a "Bridge" (Virtual Connection), block it.
    3.  **Solver:** Run **H-Search** (Circuit detection). If we find a forced win, return `value = 1.0` immediately.

---

## 5. Execution Phases (How it runs)

### Phase 1: Initialization
1.  Python script starts.
2.  Launches C++ binary.
3.  C++ binary loads `model.onnx`.
4.  C++ runs a "Warmup Inference" to check speed.
    *   If `speed > 800 evals/s`: Set `Mode = NEURAL`.
    *   Else: Set `Mode = HEURISTIC`.

### Phase 2: The Move Loop
1.  **Receive:** `CHANGE;5,5;...` (Opponent played 5,5).
2.  **Update:** Apply move to `Bitboard`.
3.  **Search:**
    *   Start 8 Threads.
    *   **Selection:** Threads descend tree using UCT.
    *   **Evaluation:**
        *   *Neural Mode:* Add state to Batcher. Wait. Batcher runs NN.
        *   *Heuristic Mode:* Run Rollout + H-Search.
    *   **Backprop:** Update stats.
4.  **Decide:** After 9.8 seconds, pick child with most `visits`.
5.  **Send:** Print `4,5` to stdout.

---

## 6. Modularity & Team Workflow

*   **Person 1 (Infra):** You own `HexAgent`, `CppAgent.cpp`, and `MyAgent.py`. You define the `Evaluator` interface.
*   **Person 2 (ML):** You own `NNEvaluator.cpp` and the Python Training Pipeline. You produce `model.onnx`.
*   **Person 3 (Engine):** You own `MCTSEngine` and `Bitboard`. You call `evaluator->evaluate()`.
*   **Person 4 (Smarts):** You own `HeuristicEvaluator.cpp`. You implement the RAVE and H-Search logic.

This decoupling means Person 2 can change the Neural Network architecture completely without breaking Person 3's MCTS code.
