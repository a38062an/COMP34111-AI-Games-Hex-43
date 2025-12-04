# Hex Communication Protocol

This document describes the text-based protocol used between the Game Engine (Python) and the Agent (C++).

## 1. Message Format

The engine sends messages to the agent via `stdin`. The agent must respond via `stdout`.

**Engine Message:**
```text
COMMAND;MOVE;BOARD;TURN;
```

**Agent Response:**
```text
row,col
```

---

## 2. Commands

| Command  | Description                                   | Example                |
| :------- | :-------------------------------------------- | :--------------------- |
| `START`  | Sent to Player 1 on the very first turn.      | `START;;000...;1;`     |
| `CHANGE` | Sent when the opponent has made a move.       | `CHANGE;5,5;000...;2;` |
| `SWAP`   | Sent on Turn 2 if the opponent chose to swap. | `SWAP;;0R0...;2;`      |

---

## 3. Data Types

### The Move (`MOVE`)
*   **Format**: `row,col` (0-indexed).
*   **Swap Move**: `-1,-1` (Only allowed on Turn 2).
*   **Note**: The engine uses `x` for Row and `y` for Column.

### The Board (`BOARD`)
*   **Format**: A single string of comma-separated rows.
*   **Values**: `0` (Empty), `R` (Red), `B` (Blue).
*   **Example (3x3)**: `000,0R0,00B`
    *   Row 0: `0 0 0`
    *   Row 1: `0 R 0`
    *   Row 2: `0 0 B`

### Full 11x11 Example
For a standard 11x11 board, the string contains 11 blocks of 11 characters, separated by commas.

**Empty Board:**
```text
00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000
```

**With Moves (Red at 0,0 and Blue at 10,10):**
```text
R0000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,00000000000,0000000000B
```

### The Turn (`TURN`)
*   **Format**: Integer starting from 1.
*   **Usage**: Used to validate the Swap Rule (only valid on Turn 2) and for time management.

---

## 4. Example Game Flow

Here is an example of the communication log for the first few turns of a game.

**Scenario**:
*   **Agent**: Red (Player 1)
*   **Opponent**: Blue (Player 2)
*   **Board Size**: 11x11

### Turn 1 (Agent's Turn)
*   **Engine Sends**: `START;;000...,000...;1;`
    *   *Meaning*: "Game Start. Board is empty. It is Turn 1."
*   **Agent Thinks**: "I want the center."
*   **Agent Responds**: `5,5`

### Turn 2 (Opponent's Turn)
*   The opponent (Blue) receives the board with Red at 5,5.
*   Opponent decides to **SWAP** (steal the move).

### Turn 3 (Agent's Turn)
*   **Engine Sends**: `SWAP;;000...,000...;3;`
    *   *Meaning*: "Opponent swapped! You are now Blue. The board is effectively empty (or mirrored). It is Turn 3."
*   **Agent Thinks**: "Okay, I am Blue now. I will play at 0,0."
*   **Agent Responds**: `0,0`

### Turn 4 (Opponent's Turn)
*   Opponent plays at `1,1`.

### Turn 5 (Agent's Turn)
*   **Engine Sends**: `CHANGE;1,1;B00...,0R0...;5;`
    *   *Meaning*: "Opponent played at 1,1. Here is the current board."
*   **Agent Thinks**: "I need to block."
*   **Agent Responds**: `1,0`

---

## 5. Running the Agent
Please refer to [README.md](README.md) for detailed instructions on how to compile and run the agent.

---

## 6. Understanding `cmd.txt`

The `cmd.txt` file is used by the tournament runner to locate your agent. It contains a single line with two parts separated by a space:

```text
agents.Group43.ExternalAgent Group43Agent
```

1.  **`agents.Group43.ExternalAgent`** (The Module Path):
    *   This tells Python where the file is located.
    *   It translates to: `agents/Group43/ExternalAgent.py`.
    *   Python uses dots `.` instead of slashes `/` for imports.

2.  **`Group43Agent`** (The Class Name):
    *   This tells Python which **Class** inside that file to use.
    *   The runner does `from agents.Group43.ExternalAgent import Group43Agent`.

**Why the space?**
The tournament runner reads this string, splits it by the space, and uses the first part to find the file and the second part to find the class to create the player object.
