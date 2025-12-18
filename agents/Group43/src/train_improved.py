import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, random_split, Dataset
import numpy as np
import argparse
import os
import time
from tqdm import tqdm
import sys

# Import our modules
sys.path.append(os.getcwd())
from Group43.src.model import HexModel
from Group43.src.dataset import HexNativeDataset

class AugmentedHexDataset(Dataset):
    """
    Wraps HexNativeDataset and applies random D2 + Transpose symmetries.
    Hex symmetries (11x11):
    0. Identity
    1. Rotate 180
    2. Transpose (Flip x,y) -> Requires Color Swap
    3. Rotate 180 + Transpose -> Requires Color Swap
    """
    def __init__(self, dataset):
        self.dataset = dataset
    
    def __len__(self):
        return len(self.dataset) * 4 # Virtual 4x size
        
    def __getitem__(self, idx):
        # Determine actual index and transform
        original_idx = idx // 4
        transform_idx = idx % 4
        
        sample = self.dataset[original_idx] # {'feature': (6,11,11), 'policy': (121,), 'value': scalar}
        
        feature = sample['feature'] # Tensor
        policy = sample['policy']   # Tensor (121)
        value = sample['value']     # Tensor scalar
        
        # Reshape policy to 11x11 for geometric transforms
        policy_board = policy.view(11, 11)
        
        if transform_idx == 0:
            # Identity
            pass
            
        elif transform_idx == 1:
            # Rotate 180
            feature = torch.flip(feature, [1, 2])
            policy_board = torch.flip(policy_board, [0, 1])
            # Value unchanged
            
        elif transform_idx == 2:
            # Transpose (Swap dims 1, 2 for feature, 0, 1 for policy)
            feature = torch.transpose(feature, 1, 2)
            policy_board = torch.transpose(policy_board, 0, 1)
            
            # COLOR SWAP REQUIRED for Hex Transpose to be valid strategy
            # Feature: Swap plane 0 (P1) and 1 (P2)
            # Plane 2 is empty (unchanged)
            # We must swap channel 0 and 1
            feature_clone = feature.clone()
            feature[0] = feature_clone[1]
            feature[1] = feature_clone[0]
            
            # Value Flip: P1 win becomes P1 loss (if we assume perspective swap)
            # Actually, if we swap colors, we are switching perspective.
            # If the original state was "Red to move, Red winning (+1)",
            # The transposed state (Red connects Top-Bottom -> Left-Right) makes Red play Blue's game.
            # So Transpose maps "Red State" to "Blue State".
            # If we swap colors (Red stones become Blue stones), 
            # then it is "Blue to move, Blue winning".
            # Since our network always predicts for "Current Player", value is +1 (Self winning).
            # So value is unchanged relative to "Current Player"?
            # Yes. "Current Player" is winning in both isomorphic states.
            pass
            
        elif transform_idx == 3:
            # Rotate 180 + Transpose
            feature = torch.flip(feature, [1, 2])
            policy_board = torch.flip(policy_board, [0, 1])
            
            feature = torch.transpose(feature, 1, 2)
            policy_board = torch.transpose(policy_board, 0, 1)
            
            # Color Swap
            feature_clone = feature.clone()
            feature[0] = feature_clone[1]
            feature[1] = feature_clone[0]
        
        return {
            'feature': feature,
            'policy': policy_board.flatten(),
            'value': value
        }

def train(args):
    device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")
    if torch.cuda.is_available(): device = torch.device("cuda")
    print(f"Using device: {device}")

    print("Loading dataset...")
    base_dataset = HexNativeDataset(args.data_dir)
    
    # Wrap with Augmentation
    full_dataset = AugmentedHexDataset(base_dataset)
    print(f"Dataset Augmented 4x: {len(base_dataset)} -> {len(full_dataset)}")
    
    total_size = len(full_dataset)
    train_size = int(0.95 * total_size)
    val_size = total_size - train_size
    train_dataset, val_dataset = random_split(full_dataset, [train_size, val_size])
    
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True, num_workers=args.workers, pin_memory=True)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False, num_workers=args.workers, pin_memory=True)
    
    model = HexModel().to(device)
    optimizer = optim.AdamW(model.parameters(), lr=args.lr, weight_decay=1e-4) # AdamW is good
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min', factor=0.5, patience=3, verbose=True)
    
    policy_criterion = nn.KLDivLoss(reduction='batchmean')
    value_criterion = nn.MSELoss()

    best_val_loss = float('inf')
    early_stop_counter = 0
    patience_limit = 10 # Stop if no improvement for 10 epochs
    
    for epoch in range(args.epochs):
        model.train()
        train_loss = 0.0
        
        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{args.epochs}")
        for batch in pbar:
            features = batch['feature'].to(device)
            target_policy = batch['policy'].to(device)
            target_value = batch['value'].to(device).unsqueeze(1)
            
            optimizer.zero_grad()
            logits, val_pred = model(features)
            
            loss_p = policy_criterion(torch.log_softmax(logits, dim=1), target_policy)
            loss_v = value_criterion(val_pred, target_value)
            loss = loss_p + loss_v
            
            loss.backward()
            optimizer.step()
            train_loss += loss.item()
            pbar.set_postfix({'L': loss.item()})
            
        # Validation
        model.eval()
        val_loss = 0.0
        with torch.no_grad():
            for batch in val_loader:
                features = batch['feature'].to(device)
                target_policy = batch['policy'].to(device)
                target_value = batch['value'].to(device).unsqueeze(1)
                logits, val_pred = model(features)
                loss_p = policy_criterion(torch.log_softmax(logits, dim=1), target_policy)
                loss_v = value_criterion(val_pred, target_value)
                val_loss += (loss_p + loss_v).item()
                
        avg_val_loss = val_loss / len(val_loader)
        avg_train_loss = train_loss / len(train_loader)
        
        print(f"Epoch {epoch+1}: Train={avg_train_loss:.4f}, Val={avg_val_loss:.4f}")
        
        scheduler.step(avg_val_loss)
        
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            early_stop_counter = 0
            torch.save(model.state_dict(), os.path.join(args.save_dir, "best_model.pth"))
            print("Saved New Best Model.")
        else:
            early_stop_counter += 1
            if early_stop_counter >= patience_limit:
                print("Early Stopping Triggered.")
                break

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--data_dir', type=str, default='agents/Group43/src/hex_dataset_raw/hex3_27x_b28.bin.gz/tdata')
    parser.add_argument('--save_dir', type=str, default='agents/Group43/src/checkpoints')
    parser.add_argument('--epochs', type=int, default=100) # Increased default
    parser.add_argument('--batch_size', type=int, default=128) # Smaller batch for regularization?
    parser.add_argument('--lr', type=float, default=1e-3)
    parser.add_argument('--workers', type=int, default=4)
    args = parser.parse_args()
    
    os.makedirs(args.save_dir, exist_ok=True)
    train(args)
