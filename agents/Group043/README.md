# Group 43 Hex Agent

A C++17 **Monte Carlo Tree Search** agent for 11×11 Hex, with RAVE, a Zobrist-hashed transposition table, pattern-weighted ("heavy") playouts, opening and swap heuristics, and dynamic time management. A thin Python wrapper connects it to the course's game engine.

## Architecture

```text
Hex.py / HexTournament.py  (course engine, Python)
        │  Agents.py: spawns the binary, sends one line per turn:
        │  "START|CHANGE|SWAP;<last move>;<board rows>;<turn>;"
        ▼
bin/CppAgent  (C++)
  CppAgent.cpp   entry point: ./CppAgent <R|B> <board_size> <time_limit_ms>
  HexAgent.cpp   protocol parsing, opening/swap logic, time management
  MCTS.cpp/.h    search: UCT + RAVE, transposition table, heavy playouts
  Node.h         tree node (1-byte move index, expanded-move bitset, UCT/RAVE stats)
  Bitboard.h     two std::bitset<121> boards, precomputed adjacency, flood-fill win check
  PatternWeights.h  4096-entry local-pattern weight table for playouts
```

### Search (`MCTS.cpp`)

- **Selection:** UCT (c = 1.414) blended with RAVE/AMAF values. The blend weight is β = √(k / (3n + k)) with k = 1000, so RAVE dominates early and UCT takes over as visits grow.
- **Expansion:** picks one untried move at random from the bitset `~(red | blue) & ~expanded`.
- **Transposition table:** 1M entries, keyed by an incrementally updated **Zobrist hash**. New nodes start from cached win and visit counts, so knowledge is shared across transpositions. The table is static and persists across moves in a game.
- **Heavy playouts:** each candidate move is scored from its 6-neighbour pattern. Each neighbour is empty, own stone, or opponent stone/wall, encoded in 2 bits, giving a 12-bit index into a 4096-entry weight table. Playouts use rejection sampling: up to 4 random candidates, each accepted with probability `weight / 255`. Bridge-like and connecting shapes are therefore preferred over uniformly random moves. Playouts fill the board, since Hex has no draws, and record each side's moves for the RAVE update.
- **Move choice:** the most-visited root child once the time budget runs out. The clock is checked every 64 iterations.

### Game logic (`HexAgent.cpp`)

- **Opening (Red):** plays one of three known "fair" opening cells, so the opponent gains little from swapping.
- **Swap decision (Blue, move 1):** swaps if Red's opening stone lands in the strong central 7×7 region or an obtuse corner.
- **Time management:**
  - 1 s per move for the first 4 moves.
  - After that, `(remaining − 1.5 s reserve) / expected moves left × 2.3`, with about 30 moves expected per player.
  - Capped at 50% of the remaining budget and floored at 100 ms.
  - Time actually spent is measured and subtracted from the budget.

## Build and run

From the repository root (inside the course Docker image, or any Linux/macOS machine with g++ and Python 3.10+):

```bash
make -C agents/Group043                 # -> agents/Group043/bin/CppAgent  (-O3 -march=native -flto)

# Play against the course's naive agent
python3 Hex.py -p1 "agents.Group043.Agents FinalAgent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -v
```

`cmd.txt` registers `FinalAgent` (30 s total time budget) for `HexTournament.py`. `BaselineAgent` and `ExperimentalAgent` (built by `make dev` as `bin/DevAgent`) are used to compare a feature branch against `main`.

## Benchmarks

| Tool | Purpose |
|---|---|
| `benchmarks/RunSPS.sh` → `SPS.cpp` | Raw search speed: MCTS simulations per second from the empty board (5 s run after a warm-up) |
| `benchmarks/RunTournament.sh` | Builds `main` as the baseline and the current branch as the dev agent (stashing local changes), then plays N games between them |
| `benchmarks/OneVsOne.py` | Head-to-head series between any two agent classes, alternating colours, with CSV logging |
| `benchmarks/benchmark_diminishing_results.py`, `plot_diminishing_results.py` | Win rate and actual time used as the total time budget varies, against a fixed-budget baseline. Results are in `plots/` |

## Contributors

Built by Group 43. On `main`:

- **Anthony Nguyen:** the core MCTS/RAVE/transposition-table engine.
- **Alex Mote:** the heavy pattern-based playouts, the bitboard and engine performance work, and the SPS and tournament benchmarking tools.

A CNN policy network and an H-search virtual-connection solver were prototyped on other branches (`CNN*`, `VC*`) and were not merged into the final agent.
