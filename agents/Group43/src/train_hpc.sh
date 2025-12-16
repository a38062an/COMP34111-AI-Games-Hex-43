#!/bin/bash
#SBATCH --job-name=hex_train
#SBATCH --output=hex_train_%j.out
#SBATCH --error=hex_train_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=12
#SBATCH --gres=gpu:2          # Request 2 GPUs
#SBATCH --mem=32G             # Increased memory for more workers
#SBATCH --time=02:00:00       # 2 hour limit (adjust as needed)

# ==============================================================================
# HEX AGENT TRAINING - SLURM SUBMISSION SCRIPT
# ==============================================================================

# 1. Load Modules (Adjust these names to match your cluster, e.g., 'python/3.9')
# module load python/3.10.12
# module load cuda/11.8

# 2. Activate Virtual Environment (if used)
# source ~/my_env/bin/activate

# 3. Print Info
echo "Running on host: $(hostname)"
echo "Starting at: $(date)"
echo "GPU Info:"
nvidia-smi

# 4. Run Training
# Use a larger batch size for GPU (e.g., 1024) to maximize throughput.
# Ensure --data_dir points to the correct location on the HPC filesystem!

python3 agents/Group43/src/train.py \
    --data_dir agents/Group43/src/hex_dataset_raw/hex3_27x_b28.bin.gz/tdata \
    --save_dir agents/Group43/src/checkpoints \
    --epochs 50 \
    --batch_size 1024 \
    --lr 0.001 \
    --workers 12

echo "Training finished at: $(date)"
