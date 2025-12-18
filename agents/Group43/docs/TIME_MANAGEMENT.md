# Time Management & Experiments

## Dynamic Time Allocation
The agent uses a 4-layer safety system to manage its 5-minute (300s) time budget:

1.  **Dynamic Curve**: Allocates time based on game phase.
    *   **Opening (Moves 0-4)**: Fast play (~1s) to save time.
    *   **Middle Game**: Heaviest thought, allocating `RemainingTime / RemainingMoves`.
    *   **End Game**: fast cleanup.
2.  **Move Cap**: Limits any single move to maximum 40% of remaining time to prevent starvation.
3.  **Safety Buffer**: Reserves a 10s "hard buffer" that is never allocated.
4.  **Panic Mode**: If time drops below 1s, the agent switches to instant play (100ms) to avoid timeout.

## Configuration
The agent can be configured via environment variables (no code changes needed):

*   `G43_TIME_LIMIT`: Total time in milliseconds (default: 300000).
*   `G43_STRATEGY`: "DYNAMIC" (default) or "STATIC".

## Experiments

### Benchmark: Diminishing Returns
To find the optimal time allocation (diminishing returns), run the benchmark script:

```bash
python3 agents/Group43/experiments/benchmark_diminishing_returns.py
```

This runs specific calibration games using the internal `src.Game` engine, pitting a Variable-Time agent against a robust Baseline to find the "knee" of the performance curve.

## Scoring
The optimization target is the tournament score:
`Score = (0.75 * WinRate) + (0.25 * SpeedProportion)`

Where `SpeedProportion` is `(TimeLimit - TimeUsed) / TimeLimit`.
Faster wins yield higher scores!
