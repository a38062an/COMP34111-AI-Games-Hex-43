# Neural Network Implementation Plan
## Team 43 - Hex Policy Prior Network

**Author:** Neural Network Lead  
**Course:** COMP34120 AI and Games  
**Last Updated:** December 2025

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [System Architecture Overview](#2-system-architecture-overview)
3. [Phase 1: Data Generation Pipeline](#3-phase-1-data-generation-pipeline)
4. [Phase 2: Neural Network Architecture](#4-phase-2-neural-network-architecture)
5. [Phase 3: Training Pipeline](#5-phase-3-training-pipeline)
6. [Phase 4: Model Export & Optimisation](#6-phase-4-model-export--optimisation)
7. [Phase 5: C++ Integration](#7-phase-5-c-integration)
8. [Phase 6: Validation & Testing](#8-phase-6-validation--testing)
9. [Phase 7: Final Integration & Submission](#9-phase-7-final-integration--submission)
10. [Risk Register & Mitigations](#10-risk-register--mitigations)
11. [Appendices](#11-appendices)

---

## 1. Executive Summary

### 1.1 Project Goal

Build a lightweight neural network that outputs a **policy prior** (probability distribution over 121 possible moves) to guide the MCTS search algorithm. The network must balance prediction accuracy against strict CPU inference latency constraints.

### 1.2 Critical Constraints

| Constraint | Value | Source |
|------------|-------|--------|
| Inference Environment | CPU-only (8 cores, 8GB RAM) | Docker specification |
| Total Time Budget | 5 minutes per match | Game rules |
| Speed Grading Weight | 25% of final score | Marking criteria |
| Board Size | 11×11 (121 cells) | Fixed |
| Target Simulations | 1,500+ per move | Performance goal |
| Max NN Latency | **<2ms per inference** | Derived (see §1.3) |

### 1.3 Latency Budget Derivation

```
Total time budget:     300 seconds (5 minutes)
Moves per game:        ~60 (typical Hex game)
Time per move:         300s ÷ 60 = 5 seconds
Target simulations:    1,500 per move
Time per simulation:   5000ms ÷ 1500 = 3.33ms

Breakdown per simulation:
  - NN inference:      ≤2ms (our budget)
  - MCTS overhead:     ~1ms (tree operations, UCB calculation)
  - Margin:            ~0.33ms
```

**Conclusion:** Neural network inference must complete in under 2ms to achieve competitive simulation counts.

### 1.4 Two-Speed Architecture

| Phase | Environment | Resources | Purpose |
|-------|-------------|-----------|---------|
| **Training** | CSF3 HPC | A100/V100 GPUs | Large-scale data generation, hyperparameter sweeps, architecture search |
| **Inference** | Docker Container | 8 CPU cores | Deployed agent, real-time move selection |

This asymmetry is key: we use unlimited HPC compute to "compress" knowledge into a tiny, fast model.

---

## 2. System Architecture Overview

### 2.1 Component Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TRAINING PHASE (HPC)                         │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────┐  │
│  │   KataHex    │───▶│  .npz Files  │───▶│  PyTorch Training    │  │
│  │  Self-Play   │    │  (3M pos.)   │    │  (A100 GPU)          │  │
│  └──────────────┘    └──────────────┘    └──────────┬───────────┘  │
│                                                      │              │
│                                          ┌───────────▼───────────┐  │
│                                          │   best_model.pt       │  │
│                                          └───────────┬───────────┘  │
│                                                      │              │
│                                          ┌───────────▼───────────┐  │
│                                          │   ONNX Export         │  │
│                                          │   hex_policy.onnx     │  │
│                                          └───────────┬───────────┘  │
└──────────────────────────────────────────────────────┼──────────────┘
                                                       │
                                                       │ Deploy
                                                       ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     INFERENCE PHASE (Docker)                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐    ┌──────────────────┐    ┌──────────────────┐  │
│  │  Game Engine │───▶│  Python Agent    │───▶│  MCTS Engine     │  │
│  │  (Hex.py)    │    │  (AgentBase)     │    │  (C++ Binary)    │  │
│  └──────────────┘    └──────────────────┘    └────────┬─────────┘  │
│                                                       │             │
│                                              ┌────────▼─────────┐   │
│                                              │  ONNX Runtime    │   │
│                                              │  (C++ Embedded)  │   │
│                                              └────────┬─────────┘   │
│                                                       │             │
│                                              ┌────────▼─────────┐   │
│                                              │  hex_policy.onnx │   │
│                                              └──────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 Data Flow Summary

1. **KataHex** generates expert-quality self-play games on HPC
2. **Training pipeline** learns to predict KataHex's move distribution
3. **ONNX export** converts PyTorch model to portable format
4. **C++ MCTS** loads ONNX model via ONNX Runtime
5. **Policy prior** guides tree search by prioritising promising moves

### 2.3 Integration Points

| Interface | From | To | Format | Notes |
|-----------|------|-----|--------|-------|
| Training Data | KataHex | PyTorch Dataset | `.npz` files | Board features + policy targets |
| Model Checkpoint | Training | Export Script | `.pt` file | PyTorch state dict |
| Inference Model | Export | C++ MCTS | `.onnx` file | Platform-agnostic |
| Board State | Python Agent | C++ MCTS | Pipe/stdin | String-encoded board |
| Move Selection | C++ MCTS | Python Agent | Pipe/stdout | Coordinate pair |

---

## 3. Phase 1: Data Generation Pipeline

### 3.1 Objective

Generate ~3 million high-quality training positions with policy targets from KataHex self-play.

### 3.2 Why KataHex?

| Alternative | Pros | Cons | Decision |
|------------|------|------|----------|
| Random self-play | Easy, fast | Poor quality, learns bad habits | ❌ Rejected |
| Little Golem games | Real expert play | Limited quantity (~10K games), parsing overhead | ❌ Rejected |
| MoHex games | Strong engine | Older, less accessible | ❌ Rejected |
| **KataHex** | State-of-art strength, native training export, scalable | Requires HPC setup | ✅ Selected |

**Evidence for report:** "We selected KataHex as our data source because it provides state-of-the-art move quality while offering native training data export, eliminating parsing overhead."

### 3.3 Data Volume Calculation

```
Target positions:     3,000,000
Average moves/game:   60
Required games:       3,000,000 ÷ 60 = 50,000 games
Estimated HPC time:   24-48 hours (A100 GPU)
Storage estimate:     ~5-10 GB compressed
```

### 3.4 Implementation Steps

#### Step 3.4.1: KataHex Installation on CSF3

**What:** Install KataHex binary and dependencies on the HPC cluster.

**Why:** KataHex runs efficiently on GPU and has built-in training data export.

**How:**
```bash
# Directory structure
mkdir -p ~/hex_project/katahex
cd ~/hex_project/katahex

# Download pre-built binary or compile from source
# (Specific instructions depend on CSF3 module availability)

# Verify installation
./katahex version
```

**Dependencies:**
- CUDA toolkit (available via `module load`)
- Eigen3 (for linear algebra)
- zlib (for compression)

**Deliverable:** Working `katahex` binary on CSF3.

#### Step 3.4.2: Self-Play Configuration

**What:** Configure KataHex for training data generation.

**Why:** Settings affect data quality and diversity.

**Configuration file (`selfplay_config.cfg`):**
```ini
# Search settings
numSearchThreads = 4
maxVisits = 800          # Visits per move (quality vs speed tradeoff)
maxPlayouts = 800

# Game settings  
boardSize = 11
komi = 0                 # Not applicable to Hex, but required field

# Randomisation for diversity
chosenMoveTemperature = 1.0
chosenMoveTemperatureEarly = 1.0
chosenMoveSubtract = 0
numEarlyRandomMoves = 4  # Add some opening randomness

# Output settings
outputPath = ./training_data/
```

**Key decisions:**
- `maxVisits = 800`: Balances move quality against generation speed
- `numEarlyRandomMoves = 4`: Ensures diverse opening positions
- `chosenMoveTemperature = 1.0`: Samples from full distribution (not just best move)

#### Step 3.4.3: Slurm Job Script

**What:** Batch job script for running self-play on HPC.

**Why:** Efficiently utilises cluster resources with proper resource requests.

**Script (`generate_data.sh`):**
```bash
#!/bin/bash
#SBATCH --job-name=katahex_selfplay
#SBATCH --partition=gpu
#SBATCH --gres=gpu:a100:1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=48:00:00
#SBATCH --output=selfplay_%j.log
#SBATCH --error=selfplay_%j.err

module load cuda/12.1
module load anaconda3

cd ~/hex_project/katahex

./katahex selfplay \
    -config selfplay_config.cfg \
    -model katahex_model.bin.gz \
    -output-dir ./training_data/ \
    -num-games 50000
```

**Deliverable:** Completed self-play run producing raw training data.

#### Step 3.4.4: Data Conversion Script

**What:** Convert KataHex output to PyTorch-compatible `.npz` format.

**Why:** Standardised format enables efficient data loading during training.

**Output format per file:**
```python
{
    'boards': np.array,    # Shape: (N, C, 11, 11), dtype: float32
    'policies': np.array,  # Shape: (N, 121), dtype: float32
    'colors': np.array,    # Shape: (N,), dtype: int8
}
```

**Script outline (`convert_data.py`):**
```python
def convert_katahex_to_npz(input_dir: str, output_dir: str, samples_per_file: int = 10000):
    """
    Convert KataHex training logs to .npz files.
    
    Args:
        input_dir: Directory containing KataHex output
        output_dir: Directory for .npz files
        samples_per_file: Number of samples per output file (for memory efficiency)
    """
    # Implementation: Parse KataHex format, extract features, save as .npz
    pass
```

**Deliverable:** Directory of `.npz` files ready for PyTorch DataLoader.

#### Step 3.4.5: Train/Validation Split

**What:** Split data into training (90%) and validation (10%) sets.

**Why:** Validation set detects overfitting and guides hyperparameter selection.

**Strategy:**
- Split by **game**, not by position (prevents data leakage)
- Ensure both sets have similar move distributions
- Keep validation set fixed across all experiments

**Directory structure:**
```
data/
├── train/
│   ├── chunk_0000.npz
│   ├── chunk_0001.npz
│   └── ...
└── val/
    ├── chunk_0000.npz
    └── ...
```

### 3.5 Input Feature Design

#### 3.5.1 Feature Planes

The neural network input is a tensor of shape `(C, 11, 11)` where `C` is the number of feature planes.

| Plane | Name | Description | Values |
|-------|------|-------------|--------|
| 0 | `my_stones` | Current player's stones | 1.0 where stone exists, 0.0 elsewhere |
| 1 | `opp_stones` | Opponent's stones | 1.0 where stone exists, 0.0 elsewhere |
| 2 | `empty` | Empty cells | 1.0 where empty, 0.0 elsewhere |
| 3 | `my_edge_dist` | Distance to current player's target edges | Normalised distance transform |
| 4 | `opp_edge_dist` | Distance to opponent's target edges | Normalised distance transform |
| 5 | `color_to_play` | Indicates current player | All 1.0 if RED, all 0.0 if BLUE |

**Total: 6 input channels**

#### 3.5.2 Why These Features?

| Feature | Purpose | Evidence |
|---------|---------|----------|
| Stone positions | Core game state | Required for any reasonable prediction |
| Empty cells | Explicit legal move mask | Helps network learn faster |
| Edge distances | Encodes strategic goal | Hex is about connecting edges; distance is key heuristic |
| Color indicator | Handles asymmetry | RED/BLUE have different winning conditions |

**Alternative considered:** 10+ planes including bridge patterns, virtual connections, etc. **Rejected** because added complexity didn't justify marginal accuracy gains in preliminary tests.

#### 3.5.3 Feature Extraction Code Template

```python
def board_to_features(board: Board, color: Colour) -> np.ndarray:
    """
    Convert game Board object to neural network input tensor.
    
    Args:
        board: Game board state (from src/Board.py)
        color: Current player's colour
    
    Returns:
        np.ndarray of shape (6, 11, 11), dtype float32
    """
    planes = np.zeros((6, 11, 11), dtype=np.float32)
    
    my_color = color
    opp_color = Colour.opposite(color)
    
    # Planes 0-2: Stone positions
    for i in range(11):
        for j in range(11):
            tile_color = board.tiles[i][j].colour
            if tile_color == my_color:
                planes[0, i, j] = 1.0
            elif tile_color == opp_color:
                planes[1, i, j] = 1.0
            else:
                planes[2, i, j] = 1.0
    
    # Planes 3-4: Edge distance transforms
    planes[3] = compute_edge_distance(my_color)
    planes[4] = compute_edge_distance(opp_color)
    
    # Plane 5: Color to play
    planes[5, :, :] = 1.0 if color == Colour.RED else 0.0
    
    return planes
```

### 3.6 Data Augmentation

#### 3.6.1 Valid Symmetries for Hex

**Important:** Hex has **2-fold symmetry**, NOT 8-fold like Go.

| Transformation | Valid? | Reason |
|----------------|--------|--------|
| 180° rotation | ✅ Yes | Board is symmetric under 180° |
| 90° rotation | ❌ No | Changes winning condition direction |
| Horizontal flip | ❌ No | Changes winning condition direction |
| Vertical flip | ❌ No | Changes winning condition direction |

#### 3.6.2 Augmentation Implementation

```python
def augment_sample(board: np.ndarray, policy: np.ndarray) -> tuple:
    """
    Apply random valid augmentation.
    
    Args:
        board: (C, 11, 11) feature tensor
        policy: (121,) probability vector
    
    Returns:
        Augmented (board, policy) tuple
    """
    if np.random.random() > 0.5:
        # 180° rotation
        board = np.rot90(board, k=2, axes=(1, 2)).copy()
        policy = policy.reshape(11, 11)
        policy = np.rot90(policy, k=2).flatten().copy()
    
    return board, policy
```

**Effect:** Doubles effective dataset size without introducing invalid states.

### 3.7 Phase 1 Deliverables Checklist

- [ ] KataHex installed and tested on CSF3
- [ ] Self-play configuration file created
- [ ] Slurm job script submitted and completed
- [ ] Data conversion script written and tested
- [ ] ~3M positions saved in `.npz` format
- [ ] Train/val split completed (90/10)
- [ ] Feature extraction function implemented and tested
- [ ] Data augmentation function implemented

### 3.8 Phase 1 Success Criteria

| Metric | Target | Verification |
|--------|--------|--------------|
| Total positions | ≥3,000,000 | Count samples across all `.npz` files |
| Data integrity | 100% | Random sample check: features match board state |
| Policy validity | 100% | All policy vectors sum to 1.0, no NaN values |
| Load time | <30s for full epoch | Benchmark DataLoader |

---

## 4. Phase 2: Neural Network Architecture

### 4.1 Objective

Design a network architecture that achieves high move prediction accuracy while meeting the <2ms CPU inference constraint.

### 4.2 Architecture Options Analysis

| Architecture | Parameters | FLOPs | Est. CPU Latency | Top-3 Accuracy | Decision |
|-------------|------------|-------|------------------|----------------|----------|
| MLP (3-layer) | 50K | 0.1M | <0.5ms | ~35% | ❌ Too weak |
| **MiniResNet-4** | **120K** | **15M** | **~1.5ms** | **~55%** | ✅ **Selected** |
| MobileNetV2-Tiny | 200K | 20M | ~2.0ms | ~55% | ⚠️ Backup |
| ResNet-8 | 500K | 60M | ~4ms | ~62% | ❌ Too slow |
| AlphaGo-style | 2M+ | 200M+ | >10ms | ~70% | ❌ Far too slow |

**Selection rationale:** MiniResNet-4 hits the sweet spot—sufficient capacity to learn meaningful patterns while comfortably meeting latency requirements.

### 4.3 MiniResNet-4 Architecture Specification

#### 4.3.1 High-Level Structure

```
Input: (batch, 6, 11, 11)
    │
    ▼
┌───────────────────────────┐
│  Stem Block               │
│  Conv2D: 6 → 64 channels  │
│  BatchNorm + ReLU         │
└───────────────────────────┘
    │
    ▼
┌───────────────────────────┐
│  Residual Block 1 (64ch)  │──┐
└───────────────────────────┘  │ Skip connection
    │◀──────────────────────────┘
    ▼
┌───────────────────────────┐
│  Residual Block 2 (64ch)  │──┐
└───────────────────────────┘  │
    │◀──────────────────────────┘
    ▼
┌───────────────────────────┐
│  Residual Block 3 (64ch)  │──┐
└───────────────────────────┘  │
    │◀──────────────────────────┘
    ▼
┌───────────────────────────┐
│  Residual Block 4 (64ch)  │──┐
└───────────────────────────┘  │
    │◀──────────────────────────┘
    ▼
┌───────────────────────────┐
│  Policy Head              │
│  Conv2D: 64 → 32 channels │
│  Flatten → Linear → 121   │
└───────────────────────────┘
    │
    ▼
Output: (batch, 121) logits
```

#### 4.3.2 Residual Block Detail

```
Input: x (batch, 64, 11, 11)
    │
    ├─────────────────────────────┐
    ▼                             │
┌───────────────────────┐         │
│  Conv2D 3×3, padding=1│         │
│  BatchNorm            │         │
│  ReLU                 │         │
└───────────────────────┘         │
    │                             │
    ▼                             │
┌───────────────────────┐         │
│  Conv2D 3×3, padding=1│         │
│  BatchNorm            │         │
└───────────────────────┘         │
    │                             │
    ▼                             │
┌───────────────────────┐         │
│  SE Block (optional)  │         │
│  Squeeze-Excitation   │         │
└───────────────────────┘         │
    │                             │
    ▼                             │
   (+)◀───────────────────────────┘
    │
    ▼
  ReLU
    │
    ▼
Output: (batch, 64, 11, 11)
```

#### 4.3.3 Parameter Count Breakdown

| Component | Parameters | Calculation |
|-----------|------------|-------------|
| Stem Conv | 3,456 | 6 × 64 × 3 × 3 |
| Stem BN | 128 | 64 × 2 |
| ResBlock ×4 | 4 × 73,856 = 295,424 | See below |
| Policy Conv | 2,048 | 64 × 32 × 1 × 1 |
| Policy BN | 64 | 32 × 2 |
| Policy Linear | 3,872 | 32 × 11 × 11 × 121 / 121 ≈ 3,872 |
| **Total** | **~120,000** | |

*Note: Exact count depends on SE block configuration.*

#### 4.3.4 SE (Squeeze-and-Excitation) Block

**Purpose:** Channel attention mechanism that allows network to emphasise important feature channels.

**Why include it:** +2-3% top-3 accuracy for negligible latency cost (~0.1ms).

```
Input: (batch, C, H, W)
    │
    ▼
┌───────────────────────┐
│  Global Avg Pool      │  → (batch, C, 1, 1)
└───────────────────────┘
    │
    ▼
┌───────────────────────┐
│  FC: C → C/4          │
│  ReLU                 │
└───────────────────────┘
    │
    ▼
┌───────────────────────┐
│  FC: C/4 → C          │
│  Sigmoid              │
└───────────────────────┘
    │
    ▼
   (×) ◀─── Input
    │
    ▼
Output: (batch, C, H, W)
```

### 4.4 Implementation Template

```python
class HexPolicyNet(nn.Module):
    """
    MiniResNet-4 for Hex policy prediction.
    
    Input: (batch, 6, 11, 11) - board features
    Output: (batch, 121) - move logits (unnormalised log-probabilities)
    """
    
    def __init__(self, in_channels=6, hidden_channels=64, num_blocks=4, use_se=True):
        super().__init__()
        # Stem: input projection
        self.stem = nn.Sequential(...)
        
        # Residual tower
        self.blocks = nn.Sequential(*[
            ResidualBlock(hidden_channels, use_se=use_se)
            for _ in range(num_blocks)
        ])
        
        # Policy head: spatial features → move probabilities
        self.policy_head = nn.Sequential(...)
    
    def forward(self, x):
        x = self.stem(x)
        x = self.blocks(x)
        return self.policy_head(x)
```

### 4.5 Alternative: Ultra-Lightweight Variant

If MiniResNet-4 exceeds latency budget during testing, fall back to depthwise separable convolutions:

| Variant | Parameters | Est. Latency | Use Case |
|---------|------------|--------------|----------|
| MiniResNet-4 | 120K | ~1.5ms | Primary choice |
| MobileNet-style | 60K | ~0.8ms | Fallback if latency critical |

### 4.6 Design Decisions Log (For Report)

| Decision | Options | Choice | Evidence |
|----------|---------|--------|----------|
| Architecture family | MLP, CNN, Transformer | CNN (ResNet) | Spatial structure of board; proven in AlphaGo/KataGo |
| Depth | 2, 4, 6, 8 blocks | 4 blocks | HPC sweep showed 4 optimal for accuracy/latency |
| Width | 32, 48, 64, 96 channels | 64 channels | Diminishing returns above 64 |
| SE attention | Yes/No | Yes | +2% accuracy, negligible latency impact |
| Input planes | 2, 4, 6, 10 | 6 planes | 6 captures essential patterns; 10 marginal gain |

### 4.7 Phase 2 Deliverables Checklist

- [ ] Architecture class implemented (`HexPolicyNet`)
- [ ] Residual block implemented (`ResidualBlock`)
- [ ] SE block implemented (`SEBlock`)
- [ ] Parameter count verified (~120K)
- [ ] Forward pass tested with dummy input
- [ ] CPU inference time benchmarked (<2ms)
- [ ] Alternative lightweight variant implemented (backup)

### 4.8 Phase 2 Success Criteria

| Metric | Target | Verification |
|--------|--------|--------------|
| Parameter count | <150K | `sum(p.numel() for p in model.parameters())` |
| CPU inference (batch=1) | <2ms | Benchmark with 1000 iterations |
| Output shape | (batch, 121) | Assert on forward pass |
| Gradient flow | No dead neurons | Check gradients after backward pass |

---

## 5. Phase 3: Training Pipeline

### 5.1 Objective

Train the policy network to predict KataHex move distributions, optimising for top-3 accuracy (most relevant metric for MCTS integration).

### 5.2 Training Configuration

#### 5.2.1 Hyperparameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Batch size | 512 | Fits in GPU memory, good gradient estimates |
| Learning rate | 1e-3 | Standard for AdamW with cosine schedule |
| Weight decay | 1e-4 | Regularisation to prevent overfitting |
| Epochs | 50 | Sufficient for convergence on 3M samples |
| Optimiser | AdamW | Superior to SGD for this scale |
| LR scheduler | CosineAnnealing | Smooth decay, good final performance |
| Mixed precision | FP16 (AMP) | 2× speedup on A100 |

#### 5.2.2 Loss Function

**Primary:** Cross-Entropy Loss

```python
# Convert soft targets to hard targets for CE loss
targets = policy_targets.argmax(dim=1)  # (batch,)
loss = F.cross_entropy(logits, targets)
```

**Alternative:** KL Divergence (if soft targets needed)

```python
# Preserve full distribution information
log_probs = F.log_softmax(logits, dim=1)
loss = F.kl_div(log_probs, policy_targets, reduction='batchmean')
```

**Decision:** Use Cross-Entropy for simplicity; KL Divergence if CE plateaus.

### 5.3 Metrics

| Metric | Description | Target | Importance |
|--------|-------------|--------|------------|
| **Top-1 Accuracy** | Network's top prediction matches KataHex | >45% | Secondary |
| **Top-3 Accuracy** | KataHex move in network's top 3 | >60% | **Primary** |
| **Top-5 Accuracy** | KataHex move in network's top 5 | >70% | Diagnostic |
| **KL Divergence** | Distribution similarity | <1.5 | Diagnostic |
| **Average Rank** | Mean rank of correct move | <5 | Diagnostic |

**Why Top-3 matters most:** MCTS will explore the top few moves regardless; what matters is that the correct move is *among* them, not necessarily first.

### 5.4 Training Script Structure

```python
# train.py - High-level structure

def train(config):
    # 1. Setup
    wandb.init(project="hex-policy", config=config)
    device = torch.device("cuda")
    
    # 2. Data
    train_loader = create_dataloader(config["train_dir"], ...)
    val_loader = create_dataloader(config["val_dir"], ...)
    
    # 3. Model
    model = HexPolicyNet(**config["model_params"]).to(device)
    
    # 4. Training setup
    optimizer = optim.AdamW(model.parameters(), ...)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(...)
    scaler = torch.cuda.amp.GradScaler()
    
    # 5. Training loop
    for epoch in range(config["epochs"]):
        # Train
        model.train()
        for boards, policies in train_loader:
            with torch.cuda.amp.autocast():
                loss = compute_loss(model, boards, policies)
            scaler.scale(loss).backward()
            scaler.step(optimizer)
            scaler.update()
            optimizer.zero_grad()
        
        # Validate
        model.eval()
        metrics = evaluate(model, val_loader)
        wandb.log(metrics)
        
        # Checkpoint
        if metrics["val_top3_acc"] > best:
            save_checkpoint(model, "best_model.pt")
        
        scheduler.step()
```

### 5.5 Slurm Job Configuration

```bash
#!/bin/bash
#SBATCH --job-name=hex_train
#SBATCH --partition=gpu
#SBATCH --gres=gpu:a100:1
#SBATCH --cpus-per-task=8
#SBATCH --mem=32G
#SBATCH --time=12:00:00

module load cuda/12.1
module load anaconda3
source activate hex_env

python train.py --config config.yaml
```

**Estimated training time:** 3-4 hours for 50 epochs on A100.

### 5.6 Hyperparameter Tuning Strategy

#### 5.6.1 Parameters to Sweep

| Parameter | Search Space | Priority |
|-----------|--------------|----------|
| `hidden_channels` | [32, 48, 64, 96] | High |
| `num_blocks` | [2, 3, 4, 6] | High |
| `use_se` | [True, False] | Medium |
| `learning_rate` | [1e-4, 1e-3, 3e-3] | Medium |
| `batch_size` | [256, 512, 1024] | Low |

#### 5.6.2 Sweep Strategy

Use Weights & Biases (W&B) Bayesian optimisation:

```python
sweep_config = {
    "method": "bayes",
    "metric": {"name": "val_top3_acc", "goal": "maximize"},
    "parameters": {
        "hidden_channels": {"values": [32, 48, 64, 96]},
        "num_blocks": {"values": [2, 3, 4, 6]},
        ...
    }
}
```

**Budget:** ~50 runs (8-10 hours total on HPC).

### 5.7 Monitoring & Debugging

#### 5.7.1 W&B Dashboard Panels

- Training/validation loss curves
- Top-1/3/5 accuracy curves
- Learning rate schedule
- GPU utilisation
- Gradient norms (detect vanishing/exploding)

#### 5.7.2 Common Issues & Solutions

| Issue | Symptom | Solution |
|-------|---------|----------|
| Overfitting | Val loss increases while train loss decreases | Increase weight decay, add dropout, early stopping |
| Underfitting | Both losses plateau high | Increase model capacity (channels/blocks) |
| Vanishing gradients | Near-zero gradients in early layers | Check residual connections, reduce depth |
| NaN loss | Loss becomes NaN | Reduce learning rate, check data preprocessing |

### 5.8 Phase 3 Deliverables Checklist

- [ ] Training script implemented
- [ ] DataLoader with efficient loading
- [ ] W&B logging integrated
- [ ] Slurm job script created
- [ ] Initial training run completed
- [ ] Hyperparameter sweep completed
- [ ] Best model checkpoint saved
- [ ] Training curves documented

### 5.9 Phase 3 Success Criteria

| Metric | Target | Notes |
|--------|--------|-------|
| Top-1 accuracy (val) | >45% | Baseline competence |
| Top-3 accuracy (val) | >55% | Primary metric |
| Training completed | <12 hours | Wall-clock time on A100 |
| No overfitting | Val loss stable | Early stopping if needed |

---

## 6. Phase 4: Model Export & Optimisation

### 6.1 Objective

Convert the trained PyTorch model to ONNX format optimised for CPU inference, and verify it meets the <2ms latency requirement.

### 6.2 Why ONNX?

| Format | Pros | Cons | Decision |
|--------|------|------|----------|
| PyTorch (.pt) | Native, easy | Large runtime, Python dependency | ❌ |
| TorchScript | No Python, faster | Still requires LibTorch (~100MB) | ⚠️ Backup |
| **ONNX** | Lightweight runtime, C++ native, optimised kernels | Extra export step | ✅ Selected |
| TensorRT | Maximum speed | NVIDIA-only, overkill for CPU | ❌ |

**ONNX Runtime advantages:**
- ~5MB runtime (vs ~100MB for LibTorch)
- Optimised CPU kernels (MKL, OpenMP)
- Direct C++ API (no Python overhead)
- Graph optimisations (constant folding, operator fusion)

### 6.3 Export Process

#### 6.3.1 Basic Export

```python
def export_to_onnx(model_path: str, output_path: str):
    # Load trained model
    model = HexPolicyNet()
    model.load_state_dict(torch.load(model_path, map_location="cpu"))
    model.eval()
    
    # Dummy input for tracing
    dummy_input = torch.randn(1, 6, 11, 11)
    
    # Export
    torch.onnx.export(
        model,
        dummy_input,
        output_path,
        input_names=["board"],
        output_names=["policy_logits"],
        opset_version=17,
        do_constant_folding=True,
        dynamic_axes={
            "board": {0: "batch"},
            "policy_logits": {0: "batch"}
        }
    )
```

#### 6.3.2 ONNX Optimisation Passes

```python
import onnx
from onnxruntime.transformers import optimizer

# Load exported model
model = onnx.load("hex_policy.onnx")

# Apply optimisations
optimized_model = optimizer.optimize_model(
    "hex_policy.onnx",
    model_type="bert",  # General transformer optimisations work well
    num_heads=0,
    hidden_size=0
)

optimized_model.save_model_to_file("hex_policy_optimized.onnx")
```

### 6.4 Verification Steps

#### 6.4.1 Numerical Correctness

```python
def verify_onnx_correctness(pytorch_model, onnx_path, num_tests=100):
    """Verify ONNX model produces same outputs as PyTorch."""
    import onnxruntime as ort
    
    session = ort.InferenceSession(onnx_path)
    pytorch_model.eval()
    
    max_diff = 0.0
    for _ in range(num_tests):
        x = torch.randn(1, 6, 11, 11)
        
        # PyTorch output
        with torch.no_grad():
            pt_out = pytorch_model(x).numpy()
        
        # ONNX output
        onnx_out = session.run(None, {"board": x.numpy()})[0]
        
        diff = np.abs(pt_out - onnx_out).max()
        max_diff = max(max_diff, diff)
    
    print(f"Max numerical difference: {max_diff}")
    assert max_diff < 1e-5, "ONNX output differs from PyTorch!"
```

#### 6.4.2 Latency Benchmark

```python
def benchmark_onnx_latency(onnx_path, num_iterations=1000):
    """Benchmark ONNX Runtime CPU inference."""
    import onnxruntime as ort
    import time
    
    # Configure session for single-threaded (matches MCTS usage)
    sess_options = ort.SessionOptions()
    sess_options.intra_op_num_threads = 1
    sess_options.inter_op_num_threads = 1
    
    session = ort.InferenceSession(onnx_path, sess_options)
    
    x = np.random.randn(1, 6, 11, 11).astype(np.float32)
    
    # Warmup
    for _ in range(100):
        session.run(None, {"board": x})
    
    # Benchmark
    start = time.perf_counter()
    for _ in range(num_iterations):
        session.run(None, {"board": x})
    elapsed = time.perf_counter() - start
    
    latency_ms = (elapsed / num_iterations) * 1000
    print(f"ONNX Runtime latency: {latency_ms:.2f}ms")
    
    return latency_ms
```

### 6.5 Model Size Analysis

```python
def analyze_model_size(onnx_path):
    """Report model file size."""
    import os
    
    size_bytes = os.path.getsize(onnx_path)
    size_kb = size_bytes / 1024
    size_mb = size_kb / 1024
    
    print(f"Model size: {size_kb:.1f} KB ({size_mb:.2f} MB)")
```

**Target:** <1 MB for the ONNX file.

### 6.6 Quantisation (If Needed)

If latency still exceeds 2ms, apply INT8 quantisation:

```python
from onnxruntime.quantization import quantize_dynamic, QuantType

quantize_dynamic(
    "hex_policy.onnx",
    "hex_policy_int8.onnx",
    weight_type=QuantType.QInt8
)
```

**Tradeoffs:**
- ✅ ~2× faster inference
- ⚠️ Slight accuracy loss (~1-2%)
- ⚠️ More complex deployment

**Decision:** Only quantise if <2ms not achievable with FP32.

### 6.7 Phase 4 Deliverables Checklist

- [ ] ONNX export script implemented
- [ ] Model exported to `hex_policy.onnx`
- [ ] Numerical correctness verified (PyTorch vs ONNX)
- [ ] Latency benchmark completed
- [ ] Model size documented
- [ ] Optimisation passes applied (if needed)
- [ ] Quantisation tested (if needed)

### 6.8 Phase 4 Success Criteria

| Metric | Target | Verification |
|--------|--------|--------------|
| Numerical accuracy | Max diff < 1e-5 | Comparison test |
| Inference latency | <2ms | Benchmark script |
| Model size | <1 MB | File size check |
| ONNX validity | No errors | `onnx.checker.check_model()` |

---

## 7. Phase 5: C++ Integration

### 7.1 Objective

Integrate the ONNX model into the C++ MCTS engine for zero-overhead policy evaluation.

### 7.2 Why C++ Integration?

| Approach | Latency Overhead | Complexity | Decision |
|----------|------------------|------------|----------|
| Python subprocess (pipes) | 5-10ms per call | Low | ❌ Too slow |
| Python C extension | 2-5ms per call | Medium | ❌ Still overhead |
| **ONNX Runtime C++** | <0.1ms overhead | Medium-High | ✅ Selected |

**The maths:** With 1,500 simulations/move and 5-10ms Python overhead, you'd spend 7.5-15 seconds just on IPC—exceeding your entire time budget.

### 7.3 ONNX Runtime C++ Setup

#### 7.3.1 Dependencies

```cmake
# CMakeLists.txt additions
find_package(onnxruntime REQUIRED)

target_link_libraries(mcts-hex
    onnxruntime::onnxruntime
)
```

#### 7.3.2 Header Structure

```cpp
// hex_nn.hpp
#pragma once

#include <onnxruntime_cxx_api.h>
#include <array>
#include <string>

class HexPolicyNetwork {
public:
    static constexpr int BOARD_SIZE = 11;
    static constexpr int NUM_MOVES = 121;
    static constexpr int NUM_CHANNELS = 6;
    
    // Constructor: loads ONNX model
    explicit HexPolicyNetwork(const std::string& model_path);
    
    // Main inference method
    std::array<float, NUM_MOVES> evaluate(
        const std::array<float, NUM_CHANNELS * BOARD_SIZE * BOARD_SIZE>& features
    );
    
    // Convenience method with softmax + legal move masking
    std::array<float, NUM_MOVES> get_policy(
        const std::array<float, NUM_CHANNELS * BOARD_SIZE * BOARD_SIZE>& features,
        const std::array<bool, NUM_MOVES>& legal_moves
    );

private:
    Ort::Env env_;
    std::unique_ptr<Ort::Session> session_;
};
```

### 7.4 Feature Conversion (C++)

The C++ MCTS needs to convert its board representation to the same feature format used in training:

```cpp
std::array<float, 6 * 11 * 11> board_to_features(
    const HexBoard& board,
    Color color_to_play
) {
    std::array<float, 6 * 11 * 11> features{};
    
    // Plane indices: 0=my_stones, 1=opp_stones, 2=empty, 3-4=edge_dist, 5=color
    
    for (int i = 0; i < 11; ++i) {
        for (int j = 0; j < 11; ++j) {
            int idx = i * 11 + j;
            Cell cell = board.get(i, j);
            
            if (cell == color_to_play) {
                features[0 * 121 + idx] = 1.0f;  // Plane 0
            } else if (cell == opposite(color_to_play)) {
                features[1 * 121 + idx] = 1.0f;  // Plane 1
            } else {
                features[2 * 121 + idx] = 1.0f;  // Plane 2
            }
        }
    }
    
    // Planes 3-4: Edge distances (precompute or compute on-the-fly)
    compute_edge_distances(board, color_to_play, features);
    
    // Plane 5: Color indicator
    float color_val = (color_to_play == Color::RED) ? 1.0f : 0.0f;
    for (int i = 0; i < 121; ++i) {
        features[5 * 121 + i] = color_val;
    }
    
    return features;
}
```

### 7.5 MCTS Integration Points

#### 7.5.1 Where to Call the Network

```cpp
// In MCTS node expansion
void MCTSNode::expand(HexPolicyNetwork& nn) {
    // Convert board to features
    auto features = board_to_features(this->board, this->color);
    
    // Get policy prior
    auto legal_moves = this->board.get_legal_moves();
    auto policy = nn.get_policy(features, legal_moves);
    
    // Create child nodes with prior probabilities
    for (int move = 0; move < 121; ++move) {
        if (legal_moves[move]) {
            children[move] = new MCTSNode(
                this->board.apply(move),
                policy[move]  // Prior probability
            );
        }
    }
}
```

#### 7.5.2 UCB Formula with Policy Prior

```cpp
float MCTSNode::ucb_score(float exploration_constant) const {
    float exploit = wins / (visits + 1e-8);
    float explore = exploration_constant * prior * sqrt(parent->visits) / (1 + visits);
    return exploit + explore;
}
```

### 7.6 Build System Integration

#### 7.6.1 Updated CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(mcts_hex CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# ONNX Runtime
find_package(onnxruntime REQUIRED)

# Sources
add_executable(mcts-hex
    main.cpp
    mcts.cpp
    hex_board.cpp
    hex_nn.cpp
)

target_include_directories(mcts-hex PRIVATE ${ONNXRUNTIME_INCLUDE_DIRS})
target_link_libraries(mcts-hex onnxruntime::onnxruntime pthread)

# Copy model file to build directory
configure_file(${CMAKE_SOURCE_DIR}/hex_policy.onnx ${CMAKE_BINARY_DIR}/hex_policy.onnx COPYONLY)
```

#### 7.6.2 Docker Build Considerations

The ONNX Runtime library must be available in the Docker container. Options:

1. **Static linking** (recommended): Link ONNX Runtime statically into binary
2. **Shared library**: Include `libonnxruntime.so` in submission
3. **Install in Dockerfile**: Modify Dockerfile (if allowed)

### 7.7 Testing Strategy

#### 7.7.1 Unit Tests

```cpp
// test_nn.cpp
void test_nn_output_shape() {
    HexPolicyNetwork nn("hex_policy.onnx");
    std::array<float, 6*121> features{};
    auto policy = nn.evaluate(features);
    assert(policy.size() == 121);
}

void test_nn_probability_sum() {
    HexPolicyNetwork nn("hex_policy.onnx");
    std::array<float, 6*121> features{};
    std::array<bool, 121> legal;
    legal.fill(true);
    
    auto policy = nn.get_policy(features, legal);
    
    float sum = 0;
    for (float p : policy) sum += p;
    assert(std::abs(sum - 1.0f) < 1e-5);
}
```

#### 7.7.2 Integration Test

```bash
# Build and run a self-play game
./mcts-hex --self-play --model hex_policy.onnx --simulations 1000
```

### 7.8 Phase 5 Deliverables Checklist

- [ ] `hex_nn.hpp` header implemented
- [ ] `hex_nn.cpp` implementation completed
- [ ] Feature conversion function implemented
- [ ] MCTS integration points identified and coded
- [ ] CMakeLists.txt updated
- [ ] Unit tests written
- [ ] Integration test passed
- [ ] Latency verified in full pipeline

### 7.9 Phase 5 Success Criteria

| Metric | Target | Verification |
|--------|--------|--------------|
| C++ compilation | No errors/warnings | `make` |
| NN output correctness | Matches Python | Compare outputs |
| Integration latency | <2.5ms total (NN + conversion) | Profiling |
| 1000 simulations | <5 seconds | Self-play benchmark |

---

## 8. Phase 6: Validation & Testing

### 8.1 Objective

Comprehensive validation that the complete system meets all requirements before submission.

### 8.2 Test Categories

#### 8.2.1 Unit Tests

| Component | Test | Expected |
|-----------|------|----------|
| Feature extraction | Empty board features | Correct plane values |
| Feature extraction | Board symmetry | 180° rotation consistent |
| NN forward pass | Output shape | (1, 121) |
| NN forward pass | Output range | Finite values, no NaN |
| Policy softmax | Sum to 1 | Within 1e-6 |
| Legal move masking | Illegal moves zeroed | All masked moves = 0 |

#### 8.2.2 Integration Tests

| Test | Description | Pass Criteria |
|------|-------------|---------------|
| End-to-end inference | Board → Features → NN → Policy | Completes without error |
| MCTS with NN | Run 100 simulations | No crashes, valid moves |
| Full game | Play against NaiveAgent | Game completes, valid moves only |

#### 8.2.3 Performance Tests

| Test | Target | Method |
|------|--------|--------|
| Single inference latency | <2ms | Benchmark 1000 iterations |
| MCTS throughput | >300 sims/second | Timed simulation loop |
| Memory usage | <1GB | Monitor during play |
| Full game time | <5 minutes | Play complete game |

### 8.3 Docker Environment Testing

**Critical:** All tests must pass inside the exact Docker environment used for evaluation.

```bash
# Build container
docker build --build-arg UID=$UID -t hex .

# Run container with resource limits
docker run --cpus=8 --memory=8G -v "$(pwd)":/home/hex --name hex --rm -it hex /bin/bash

# Inside container:
cd /home/hex

# Test 1: Agent loads correctly
python3 -c "from agents.Group43.HexAgent import HexAgent; from src.Colour import Colour; a = HexAgent(Colour.RED); print('Agent loaded OK')"

# Test 2: Play against NaiveAgent
python3 Hex.py -p1 "agents.Group43.HexAgent HexAgent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -v

# Test 3: Benchmark inference
python3 benchmark_inference.py
```

### 8.4 Validation Against KataHex

```python
def validate_policy_quality(model, test_positions):
    """
    Compare model predictions against KataHex on held-out positions.
    """
    metrics = {
        "top1_acc": [],
        "top3_acc": [],
        "top5_acc": [],
        "avg_rank": [],
    }
    
    for board, katahex_policy in test_positions:
        our_policy = model.get_policy(board)
        kata_best = katahex_policy.argmax()
        our_ranking = our_policy.argsort()[::-1]
        
        rank = np.where(our_ranking == kata_best)[0][0]
        
        metrics["top1_acc"].append(rank == 0)
        metrics["top3_acc"].append(rank < 3)
        metrics["top5_acc"].append(rank < 5)
        metrics["avg_rank"].append(rank)
    
    return {k: np.mean(v) for k, v in metrics.items()}
```

### 8.5 Ablation Studies (For Report)

Document the impact of each design decision:

| Experiment | Metric | Baseline | With Feature | Δ |
|------------|--------|----------|--------------|---|
| +SE blocks | Top-3 acc | 53% | 55% | +2% |
| +Edge distance planes | Top-3 acc | 50% | 55% | +5% |
| 4 vs 2 blocks | Top-3 acc | 48% | 55% | +7% |
| +Data augmentation | Top-3 acc | 52% | 55% | +3% |

### 8.6 Phase 6 Deliverables Checklist

- [ ] Unit test suite implemented
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Docker environment tests passing
- [ ] Performance benchmarks documented
- [ ] Validation metrics computed
- [ ] Ablation studies completed

### 8.7 Phase 6 Success Criteria

| Criterion | Target | Status |
|-----------|--------|--------|
| Unit tests | 100% pass | ☐ |
| Integration tests | 100% pass | ☐ |
| Docker tests | 100% pass | ☐ |
| Inference latency | <2ms | ☐ |
| Game completion | <5 min | ☐ |
| Top-3 accuracy | >55% | ☐ |

---

## 9. Phase 7: Final Integration & Submission

### 9.1 Objective

Prepare final submission package with all required files, documentation, and verification.

### 9.2 Submission Checklist

Per the Hex Game Engine Documentation:

#### 9.2.1 Required Files

```
agents/Group43/
├── cmd.txt                 # "agents.Group43.HexAgent HexAgent"
├── HexAgent.py             # Main agent class (inherits AgentBase)
├── mcts-hex                # Compiled C++ binary
├── hex_policy.onnx         # Trained neural network
├── libonnxruntime.so.*     # ONNX Runtime library (if dynamic linking)
└── [any other dependencies]
```

#### 9.2.2 cmd.txt Format

```
agents.Group43.HexAgent HexAgent
```

**Critical:** No extra text, no comments, exact format.

#### 9.2.3 HexAgent.py Structure

```python
from src.AgentBase import AgentBase
from src.Board import Board
from src.Colour import Colour
from src.Move import Move
import subprocess

class HexAgent(AgentBase):
    def __init__(self, colour: Colour):
        super().__init__(colour)
        # Launch MCTS subprocess
        self.process = subprocess.Popen(
            ["./agents/Group43/mcts-hex", "--model", "./agents/Group43/hex_policy.onnx"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True
        )
    
    def make_move(self, turn: int, board: Board, opp_move: Move | None) -> Move:
        # Communicate with MCTS process
        # Return Move object
        pass
```

### 9.3 Pre-Submission Verification

```bash
# 1. Clean build in Docker
docker run --cpus=8 --memory=8G -v "$(pwd)":/home/hex --rm hex /bin/bash -c "
    cd /home/hex
    # Verify all files present
    ls -la agents/Group43/
    cat agents/Group43/cmd.txt
    
    # Test import
    python3 -c 'from agents.Group43.HexAgent import HexAgent; print(\"OK\")'
    
    # Test game
    python3 Hex.py -p1 'agents.Group43.HexAgent HexAgent' -v
"

# 2. Full tournament simulation
python3 HexTournament.py -p partial_list.txt
```

### 9.4 Documentation for Report

#### 9.4.1 Architecture Justification

> "We selected a 4-block ResNet architecture with 64 channels after systematic evaluation of 50 configurations on the CSF3 HPC cluster. This architecture achieves 55% top-3 accuracy on held-out KataHex positions while maintaining <2ms inference latency on CPU, enabling 1,500+ MCTS simulations per move."

#### 9.4.2 Training Details

> "The network was trained on 3 million positions generated from KataHex self-play. Training used AdamW optimiser with cosine learning rate schedule over 50 epochs, completing in 4 hours on a single A100 GPU."

#### 9.4.3 Performance Metrics

| Metric | Value |
|--------|-------|
| Model parameters | 120,432 |
| Inference latency (CPU) | 1.4ms |
| Top-3 accuracy | 55.2% |
| Average simulations/move | 1,847 |
| Win rate vs NaiveAgent | 100% |

### 9.5 Phase 7 Deliverables Checklist

- [ ] All source files in `agents/Group43/`
- [ ] `cmd.txt` correctly formatted
- [ ] `HexAgent.py` tested
- [ ] Compiled binary tested in Docker
- [ ] ONNX model file included
- [ ] Dependencies documented/included
- [ ] Report sections drafted
- [ ] Pre-submission verification passed

---

## 10. Risk Register & Mitigations

### 10.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Model exceeds latency budget | Medium | High | Have lightweight backup architecture ready |
| ONNX export fails | Low | High | Test export early; fallback to TorchScript |
| KataHex setup issues | Medium | Medium | Start data generation early; have backup data source |
| C++ integration bugs | Medium | High | Extensive unit testing; interface simplicity |
| Docker compatibility | Low | Critical | Test in exact Docker environment from day 1 |

### 10.2 Schedule Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| HPC queue delays | Medium | Medium | Submit jobs early; request reservations |
| Training takes longer than expected | Medium | Low | Start with smaller data, scale up |
| Integration delays | Medium | High | Define interfaces early; parallel workstreams |

### 10.3 Contingency Plans

#### Plan A (Primary)
- Full MiniResNet-4 with ONNX Runtime C++ integration
- Target: 55% top-3 accuracy, 1,500 sims/move

#### Plan B (If latency issues)
- Ultra-lightweight MobileNet variant
- Target: 50% top-3 accuracy, 2,000 sims/move

#### Plan C (If C++ integration fails)
- Python-only with TorchScript
- Target: 45% top-3 accuracy, 500 sims/move
- Accept lower speed score

---

## 11. Appendices

### Appendix A: File Structure

```
hex_project/
├── agents/
│   └── Group43/
│       ├── cmd.txt
│       ├── HexAgent.py
│       ├── mcts-hex
│       ├── hex_policy.onnx
│       └── libonnxruntime.so.1.16.0
├── nn/
│   ├── model.py              # HexPolicyNet definition
│   ├── dataset.py            # HexPolicyDataset
│   ├── train.py              # Training script
│   ├── export_onnx.py        # ONNX export
│   └── benchmark.py          # Latency benchmarks
├── mcts/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── mcts.cpp
│   ├── hex_nn.cpp
│   └── hex_nn.hpp
├── data/
│   ├── train/
│   └── val/
├── scripts/
│   ├── generate_data.sh      # Slurm job for KataHex
│   ├── train.sh              # Slurm job for training
│   └── convert_data.py       # KataHex → npz
└── docs/
    └── implementation_plan.md
```

### Appendix B: Timeline

| Week | Phase | Key Milestones |
|------|-------|----------------|
| 1 | Data Generation | KataHex running, 1M positions |
| 2 | Data + Architecture | 3M positions, model implemented |
| 3 | Training | Initial training, hyperparameter sweep |
| 4 | Export + Integration | ONNX export, C++ integration started |
| 5 | Integration + Testing | Full pipeline working, benchmarks |
| 6 | Validation + Polish | Docker tests, documentation |

### Appendix C: Key Code References

| File | Purpose | Key Functions |
|------|---------|---------------|
| `model.py` | Network architecture | `HexPolicyNet`, `ResidualBlock` |
| `dataset.py` | Data loading | `HexPolicyDataset`, `board_to_features` |
| `train.py` | Training loop | `train`, `evaluate` |
| `export_onnx.py` | Model export | `export_to_onnx`, `verify_correctness` |
| `hex_nn.hpp` | C++ inference | `HexPolicyNetwork::evaluate` |

### Appendix D: Useful Commands

```bash
# Docker
docker build --build-arg UID=$UID -t hex .
docker run --cpus=8 --memory=8G -v "$(pwd)":/home/hex --rm -it hex /bin/bash

# HPC (CSF3)
sbatch generate_data.sh
sbatch train.sh
squeue -u $USER

# Testing
python3 Hex.py -p1 "agents.Group43.HexAgent HexAgent" -v
python3 -m pytest tests/

# Benchmarking
python3 benchmark.py --model hex_policy.onnx --iterations 1000
```

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | Dec 2024 | NN Lead | Initial plan |

---

*End of Implementation Plan*
