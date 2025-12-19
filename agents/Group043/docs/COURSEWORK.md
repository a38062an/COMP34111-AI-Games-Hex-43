# Optimizations & Assessment Details

This document outlines the specific technical optimizations implemented to maximize the performance of the Group 43 Hex Agent. These implementations fulfill the "Core Search Engine" requirements outlined in our Group Proforma.

## 1. Technical Optimizations ("Clever Software Engineering")

To support the computationally expensive future goal of a Hybrid Neural MCTS[cite: 8], the base C++ engine was optimized to achieve maximum **Simulations Per Second (SPS)**. We have implemented the following features to hit the "Extras" marking criteria.

### A. Transposition Table with Zobrist Hashing

- **The Problem:** In Hex, move order is commutative (A then B == B then A). Standard MCTS treats these as distinct branches, wasting time re-solving identical positions.
- **The Solution:**
  - **Zobrist Hashing:** We assigned random 64-bit integers to every tile/player combination. The board hash is the XOR sum of these integers, allowing $O(1)$ incremental updates during tree traversal.
  - **Transposition Table:** A fixed-size hash table caches `wins` and `visits`.
- **Result:** This allows the MCTS to "teleport" knowledge between branches, effectively increasing search depth.

### B. Linear Memory Pool (`NodePool`)

- **The Problem:** Standard `new Node()` calls involve syscalls to the OS memory manager. This is slow and causes heap fragmentation (nodes scattered in memory), leading to poor CPU cache locality.
- **The Solution:** We implemented a custom linear memory pool using `std::deque`.
  - Nodes are allocated contiguously in memory pages.
  - **Pointer Stability:** Unlike `std::vector`, `std::deque` guarantees pointers remain valid as the pool grows.
  - **Instant Reset:** The entire search tree is de-allocated in $O(1)$ time by simply resetting the pool index.
- **Result:** Eliminates allocation overhead in the critical expansion path.

### C. Bitboards and Fast Simulation

- **The Problem:** `char board[11][11]` arrays require $O(N)$ copying and slow DFS for connectivity checks.
- **The Solution:**
  - **Storage:** Two `std::bitset<121>` variables representing Red and Blue pieces.
  - **Optimization:** We utilize a fixed-size stack array `int moves[121]` with a "Swap-Remove" strategy to pick random moves during simulation, avoiding all heap allocations.
- **Result:** A ~4.5% increase in raw Nodes Per Second (NPS) compared to the unoptimized baseline.

---

## 2. Algorithm Implementation: RAVE

As specified in our division of labor, we have implemented **Rapid Action Value Estimation (RAVE)**.

- **Theory:** RAVE assumes that if a move is good, it is likely good regardless of _when_ it is played in a sequence.
- **Implementation:**
  - During a simulation, we track _all_ moves made by the winner.
  - We update the `raveWins` and `raveVisits` for the corresponding child nodes at the root.
  - The final node selection score is a weighted average of UCT (exact) and RAVE (heuristic) values.
- **Impact:** RAVE accelerates learning in the early phases of the search (low visit counts), allowing the agent to converge on promising moves significantly faster than standard UCT.

---

## 3. Experimental Evidence

We conducted A/B testing to validate our current implementation status.

### Experiment 1: RAVE vs UCT (Algorithmic Strength)

We ran 20 games between a RAVE-enabled agent ($k=1000$) and a standard UCT agent ($k=0$) with a 50ms time limit per move.

- **Result:** RAVE achieved a **100% Win Rate (20/20)**.
- **Conclusion:** RAVE successfully identifies critical moves faster than UCT, fulfilling the search acceleration requirement of the proforma.

### Experiment 2: Transposition Table Impact

We benchmarked the agent's playing strength with and without the TT enabled against a fixed opponent.

- **Without TT:** 24% Win Rate.
- **With TT:** 47% Win Rate.
- **Conclusion:** The TT nearly doubles the agent's effective strength by preventing redundant calculations.

---

## 4. Literature & References

Our design decisions are grounded in the academic literature cited in our proforma[cite: 1, 8]:

1.  **Gelly & Silver (2011)**: _Monte-Carlo tree search and RAVE_. (Justification for RAVE ).
2.  **Arneson et al. (2010)**: _Monte Carlo Tree Search in Hex_. (Justification for H-Search and Virtual Connections [cite: 11]).
3.  **Enzenberger & Müller (2009)**: _Lock-free MCTS_. (Inspiration for Memory Pool/TT design).
4.  **Knuth (1975)**: _Alpha-Beta Pruning_. (Foundational concept for Zobrist Hashing).
5.  **MoHex 2.0 Papers**: The basis for our Hybrid Architecture[cite: 8].
