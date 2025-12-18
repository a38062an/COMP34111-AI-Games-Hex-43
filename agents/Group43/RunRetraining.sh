#!/bin/bash
# Master Training Script for Group43
# 1. Trains model with Data Augmentation (4x Effective Data)
# 2. Exports to TorchScript for C++ Agent

# Navigate to project root
cd "$(dirname "$0")/../../.."

echo "=================================================="
echo "Phase 1: Deep Learning Training (Improved)"
echo "=================================================="
echo "Using Data Augmentation (Rotation/Reflection) to multiply dataset by 4x."
echo "Training for up to 100 Epochs with Early Stopping."

python3 agents/Group43/src/train_improved.py \
    --epochs 100 \
    --batch_size 256 \
    --lr 0.001

echo ""
echo "=================================================="
echo "Phase 2: Exporting to C++ (TorchScript)"
echo "=================================================="

python3 agents/Group43/src/export_model.py \
    --checkpoint agents/Group43/src/checkpoints/best_model.pth \
    --output_dir agents/Group43/src/checkpoints

echo ""
echo "Done! New brain installed at: agents/Group43/src/checkpoints/hex_model.pt"
echo "You can now run the agent (it will use the new brain automatically)."
