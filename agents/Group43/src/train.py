import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, random_split
import numpy as np
import argparse
import os
import time
from tqdm import tqdm

# Import our modules
import sys
sys.path.append(os.getcwd()) # Ensure root is in path
from agents.Group43.src.model import HexModel
from agents.Group43.src.dataset import HexNativeDataset

def train(args):
    # 1. Setup Device
    device = torch.device("mps" if torch.backends.mps.is_available() else "cpu")
    if torch.cuda.is_available(): device = torch.device("cuda")
    print(f"Using device: {device}")

    # 2. Load Dataset
    print("Loading dataset...")
    full_dataset = HexNativeDataset(args.data_dir)
    
    # Split Train/Val
    total_size = len(full_dataset)
    train_size = int(0.95 * total_size)
    val_size = total_size - train_size
    train_dataset, val_dataset = random_split(full_dataset, [train_size, val_size])
    
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True, num_workers=args.workers, pin_memory=True)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False, num_workers=args.workers, pin_memory=True)
    
    print(f"Train samples: {train_size}, Val samples: {val_size}")

    # 3. Initialize Model
    model = HexModel().to(device)
    if torch.cuda.device_count() > 1:
        print(f"Using {torch.cuda.device_count()} GPUs!")
        model = nn.DataParallel(model)
    print(f"Model parameters: {sum(p.numel() for p in model.parameters())}")

    # 4. Optimizer & Loss
    optimizer = optim.AdamW(model.parameters(), lr=args.lr, weight_decay=1e-4)
    
    # Loss functions
    # Policy: KLDivLoss (since target is probability distribution)
    # Input should be log_softmax, target is probs.
    policy_criterion = nn.KLDivLoss(reduction='batchmean')
    
    # Value: MSELoss (target is -1 to 1 scalar)
    value_criterion = nn.MSELoss()

    # 5. Training Loop
    best_val_loss = float('inf')
    
    for epoch in range(args.epochs):
        model.train()
        train_loss = 0.0
        p_loss_total = 0.0
        v_loss_total = 0.0
        
        start_time = time.time()
        
        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{args.epochs}")
        for batch in pbar:
            features = batch['feature'].to(device) # (B, 6, 11, 11)
            target_policy = batch['policy'].to(device) # (B, 121)
            target_value = batch['value'].to(device).unsqueeze(1) # (B, 1)
            
            optimizer.zero_grad()
            
            # Forward
            pred_policy_logits, pred_value = model(features)
            
            # Policy Loss: LogSoftmax(logits) vs Target Probs
            # KLDiv expects log-probs
            log_probs = torch.log_softmax(pred_policy_logits, dim=1)
            loss_policy = policy_criterion(log_probs, target_policy)
            
            # Value Loss
            loss_value = value_criterion(pred_value, target_value)
            
            # Total Loss
            loss = loss_policy + loss_value
            
            loss.backward()
            optimizer.step()
            
            train_loss += loss.item()
            p_loss_total += loss_policy.item()
            v_loss_total += loss_value.item()
            
            pbar.set_postfix({'Loss': loss.item(), 'P_Loss': loss_policy.item(), 'V_Loss': loss_value.item()})
            
        # Validation
        model.eval()
        val_loss = 0.0
        with torch.no_grad():
            for batch in val_loader:
                features = batch['feature'].to(device)
                target_policy = batch['policy'].to(device)
                target_value = batch['value'].to(device).unsqueeze(1)
                
                p_logits, p_val = model(features)
                
                l_p = policy_criterion(torch.log_softmax(p_logits, dim=1), target_policy)
                l_v = value_criterion(p_val, target_value)
                
                val_loss += (l_p + l_v).item()
                
        avg_train_loss = train_loss / len(train_loader)
        avg_val_loss = val_loss / len(val_loader)
        
        print(f"Epoch {epoch+1} | Train Loss: {avg_train_loss:.4f} (P: {p_loss_total/len(train_loader):.4f}, V: {v_loss_total/len(train_loader):.4f}) | Val Loss: {avg_val_loss:.4f}")
        
        # Save Checkpoint
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            save_path = os.path.join(args.save_dir, "best_model.pth")
            torch.save(model.state_dict(), save_path)
            print(f"Saved best model to {save_path}")
            
        # Regular save
        if (epoch + 1) % 5 == 0:
            torch.save(model.state_dict(), os.path.join(args.save_dir, f"checkpoint_ep{epoch+1}.pth"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--data_dir', type=str, default='agents/Group43/src/hex_dataset_raw/hex3_27x_b28.bin.gz/tdata')
    parser.add_argument('--save_dir', type=str, default='agents/Group43/src/checkpoints')
    parser.add_argument('--epochs', type=int, default=10)
    parser.add_argument('--batch_size', type=int, default=256)
    parser.add_argument('--lr', type=float, default=1e-3)
    parser.add_argument('--workers', type=int, default=4, help='Number of data loading workers')
    args = parser.parse_args()
    
    os.makedirs(args.save_dir, exist_ok=True)
    train(args)
