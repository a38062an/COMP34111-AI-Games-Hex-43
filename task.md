# Task List: Hex AI (Dual-Mode Strategy)

## Phase 1: The Parallel Sprint (Week 1)
- [ ] **Stream A: Infrastructure (Person 1)** <!-- id: 0 -->
    - [ ] Create `Dockerfile` with C++20 & ONNX Runtime <!-- id: 1 -->
    - [ ] Implement `CppAgent` main loop & Protocol Parser <!-- id: 2 -->
    - [ ] Create `Makefile` linking `libonnxruntime` <!-- id: 3 -->
- [ ] **Stream B: The Brain (Person 2 - ML)** <!-- id: 4 -->
    - [ ] Set up PyTorch training loop on HPC <!-- id: 5 -->
    - [ ] Define `ResNet-5x64` architecture <!-- id: 6 -->
    - [ ] Implement `export_to_onnx.py` with Int8 Quantization <!-- id: 7 -->
- [ ] **Stream C: The Engine (Person 3)** <!-- id: 8 -->
    - [ ] Implement `Bitboard` class (128-bit math) <!-- id: 9 -->
    - [ ] Implement `MCTS` Node structure & UCT Formula <!-- id: 10 -->
- [ ] **Stream D: The Smarts (Person 4)** <!-- id: 11 -->
    - [ ] Implement `RAVE` statistics structure <!-- id: 12 -->
    - [ ] Implement `VirtualConnection` detector (H-Search) <!-- id: 13 -->

## Phase 2: Integration (Week 2)
- [ ] **The Merge** <!-- id: 14 -->
    - [ ] Integrate `Bitboard` into MCTS <!-- id: 15 -->
    - [ ] Integrate `ONNX Runtime` into MCTS (Mode A) <!-- id: 16 -->
    - [ ] Integrate `RAVE/VC` into MCTS (Mode B) <!-- id: 17 -->
- [ ] **Performance Testing** <!-- id: 18 -->
    - [ ] Benchmark `evals/sec` with ONNX (Target: >800) <!-- id: 19 -->
    - [ ] Benchmark `sims/sec` with Heuristics (Target: >20k) <!-- id: 20 -->

## Phase 3: Optimization & Tuning (Week 3-4)
- [ ] **Mode Selection** <!-- id: 21 -->
    - [ ] Implement runtime switch based on performance <!-- id: 22 -->
- [ ] **Tuning** <!-- id: 23 -->
    - [ ] Tune `UCT_C` for Neural Network (usually lower) <!-- id: 24 -->
    - [ ] Tune `RAVE_Bias` for Heuristic Mode <!-- id: 25 -->
- [ ] **Final Polish** <!-- id: 26 -->
    - [ ] Implement Pondering (Think on opponent's time) <!-- id: 27 -->
    - [ ] Hard-code Opening Book & Swap Logic <!-- id: 28 -->
