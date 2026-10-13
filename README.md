# COMP34111 AI and Games: Hex Agent (Group 43)

Group 43's coursework agent for 11×11 [Hex](https://en.wikipedia.org/wiki/Hex_(board_game)) with the swap (pie) rule, built on the course-provided Python game engine and tournament runner.

**Our agent is in [`agents/Group043/`](agents/Group043/).** It is a C++ Monte Carlo Tree Search engine with RAVE, a Zobrist transposition table, pattern-weighted playouts and dynamic time management. See [its README](agents/Group043/README.md) for the architecture, build steps and benchmarks.

## Repository layout

| Path | Contents |
|---|---|
| `agents/Group043/` | Our agent: C++ engine, Python wrapper, benchmarks |
| `Hex.py` | Run a single game between two agents |
| `HexTournament.py` | Round-robin tournament across every `agents/Group*/cmd.txt` |
| `src/` | Course game engine (board, rules, move validation, timing) |
| `agents/DefaultAgents/`, `agents/MCTSAgent/`, `agents/TestAgents/`, `agents/Group{1,2,997,998,999}/` | Course-provided reference and test agents (naive, cheating, crashing, external process) |
| `test/` | Engine unit tests |
| `Dockerfile` | Course evaluation environment (Python 3.11, 8 CPUs / 8 GB) |
| `Hex game doc.pdf` | Course specification of the game and the agent protocol |

## Quick start

```bash
# Optional: run inside the evaluation container
docker build --build-arg UID=$UID -t hex .
docker run --cpus=8 --memory=8G -v "$(pwd)":/home/hex --rm -it hex /bin/bash

make -C agents/Group043
python3 Hex.py -p1 "agents.Group043.Agents FinalAgent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -v
python3 HexTournament.py            # writes game_results_*.csv / game_stat_*.csv
python3 -m unittest discover        # engine tests
```
