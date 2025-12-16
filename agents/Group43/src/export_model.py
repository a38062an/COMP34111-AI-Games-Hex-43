import torch
import torch.nn as nn
import argparse
import os
import sys

sys.path.append(os.getcwd())
from agents.Group43.src.model import HexModel

def export(args):
    # 1. Load Model
    device = torch.device('cpu') # Export on CPU, safer
    model = HexModel().to(device)
    
    if not os.path.exists(args.checkpoint):
        print(f"Error: Checkpoint not found at {args.checkpoint}")
        return
        
    checkpoint = torch.load(args.checkpoint, map_location=device)
    
    # Handle DataParallel/Distributed wrapper
    if 'state_dict' in checkpoint:
        state_dict = checkpoint['state_dict']
    else:
        state_dict = checkpoint
        
    # Remove 'module.' prefix if saved with DataParallel
    new_state_dict = {}
    for k, v in state_dict.items():
        if k.startswith('module.'):
            new_state_dict[k[7:]] = v
        else:
            new_state_dict[k] = v
            
    model.load_state_dict(new_state_dict)
    model.eval()
    print("Model loaded successfully.")

    # 2. Trace Model
    # Dummy input: (Batch=1, Channels=6, Height=11, Width=11)
    dummy_input = torch.randn(1, 6, 11, 11, device=device)
    
    # We use 'strict=False' if we have dynamic control flow, but HexModel is static.
    print("Tracing model...")
    traced_script_module = torch.jit.trace(model, dummy_input)
    
    # 3. Save
    output_path = os.path.join(args.output_dir, "hex_model.pt")
    traced_script_module.save(output_path)
    print(f"Exported TorchScript model to: {output_path}")
    
    # 4. Verification
    print("Verifying export...")
    with torch.no_grad():
        py_pol, py_val = model(dummy_input)
        ts_pol, ts_val = traced_script_module(dummy_input)
        
        pol_diff = torch.max(torch.abs(py_pol - ts_pol)).item()
        val_diff = torch.max(torch.abs(py_val - ts_val)).item()
        
        print(f"Policy Max Diff: {pol_diff:.6f}")
        print(f"Value Max Diff: {val_diff:.6f}")
        
        if pol_diff < 1e-5 and val_diff < 1e-5:
            print("VERIFICATION PASSED: Outputs match.")
        else:
            print("VERIFICATION FAILED: outputs diverge!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--checkpoint', type=str, required=True, help='Path to .pth checkpoint')
    parser.add_argument('--output_dir', type=str, default='agents/Group43/src/checkpoints')
    args = parser.parse_args()
    
    os.makedirs(args.output_dir, exist_ok=True)
    export(args)
