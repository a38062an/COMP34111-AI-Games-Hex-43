import torch
from torch.utils.data import Dataset
import numpy as np
import glob
import os

class HexNativeDataset(Dataset):
    def __init__(self, data_dir, board_size=11):
        """
        Loads native KataHex .npz files.
        Data format:
        - binaryInputNCHWPacked: (N, 22, 16) uint8 -- Needs unpacking
        - policyTargetsNCMove: (N, 2, 122) int16 -- [0] is visits? [1] is policy?
        - valueTargetsNCHW: (N, 5, 11, 11) or globalTargetsNC (N, 64) float
        """
        self.files = glob.glob(os.path.join(data_dir, "*.npz"))
        self.board_size = board_size
        
        if not self.files:
            raise FileNotFoundError(f"No .npz files found in {data_dir}")
            
        print(f"Found {len(self.files)} files. Loading...")
        
        
        self.inputs_list = []
        self.policies_list = []
        self.values_list = []
        
        for f in self.files:
            try:
                d = np.load(f)
                # Input: Unpack on load to save compute during training?
                # Packed: 22*16 bytes = 352 bytes/sample. 1.2M samples = 400MB.
                # Unpacked: 22*121*4 bytes = 10KB/sample. 1.2M samples = 12GB.
                # Better to keep PACKED in RAM and unpack in __getitem__.
                self.inputs_list.append(d['binaryInputNCHWPacked']) 
                
                # Policy: (N, 2, 122). Index 0 is often 'Policy Target' or 'Visit Count'.
                # Let's assume idx 0 is the robust target distribution. 
                # Shape 122: 0-120 board, 121 pass?
                self.policies_list.append(d['policyTargetsNCMove'][:, 0, :])
                
                # Value: globalTargetsNC (N, 64). Index 0 is typically Win/Loss (-1 to 1).
                # Or valueTargetsNCHW.
                # Let's use globalTargetsNC[:, 0] as the primary Win Outcome.
                self.values_list.append(d['globalTargetsNC'][:, 0])
                
            except Exception as e:
                print(f"Skipping {f}: {e}")
                
        self.inputs = np.concatenate(self.inputs_list)
        self.policies = np.concatenate(self.policies_list)
        self.values = np.concatenate(self.values_list)
        
        print(f"Loaded {len(self.inputs)} samples.")
        print(f"Input Shape: {self.inputs.shape} (Packed)")
        print(f"Policy Shape: {self.policies.shape}")
        print(f"Value Shape: {self.values.shape}")

    def __len__(self):
        return len(self.inputs)

    def unpack(self, packed):
        # packed: (22, 16)
        # 1. Unpack to (22, 128) bits
        bits = np.unpackbits(packed, axis=1)
        # 2. Trim to (22, 121)
        bits = bits[:, :self.board_size**2]
        # 3. Reshape (22, 11, 11)
        return bits.reshape(22, self.board_size, self.board_size).astype(np.float32)

    def __getitem__(self, idx):
        # 1. Unpack Input
        # (22, 11, 11)
        features = self.unpack(self.inputs[idx])
        
        # 2. Extract needed planes
        # KataGo format often has:
        # 0: Stones of P1 (Current)
        # 1: Stones of P2 (Opponent)
        # ... History ...
        # For simplicity, take planes 0-5 as "Current State" proxy 
        # (My, Opp, Empty? No, usually binary masks).
        # To match the model input size (6), we can select 6 planes.
        # Let's assume 0=My, 1=Opp.
        # We need to construct the 6-plane format the model expects:
        # [My, Opp, Empty, MyDist, OppDist, Color]
        # Easier to ADAPT DATA to MODEL for now to keep model fixed.
        
        my_stones = features[0]
        opp_stones = features[1]
        empty = 1.0 - (my_stones + opp_stones)
        
        # Construct a simple input tensor.
        model_input = np.zeros((6, 11, 11), dtype=np.float32)
        model_input[0] = my_stones
        model_input[1] = opp_stones
        model_input[2] = empty
        # Skip Distance planes for now (or computes them on fly - fast).
        # Color plane might be in features[12] or similar.
        
        # 3. Policy Target
        # Raw policy is (122,) floats/logits.
        # Convert to label (index) if CrossEntropy, or keep distribution for KLDiv.
        # AlphaZero uses KL Divergence on the full distribution.
        # Let's use the distribution directly.
        policy_dist = self.policies[idx][:121] # Drop 'pass' if hex doesn't support pass
        # Normalize sum to 1?
        policy_sum = np.sum(policy_dist)
        if policy_sum > 0:
            policy_dist = policy_dist / policy_sum
        
        return {
            'feature': torch.from_numpy(model_input),
            'policy': torch.from_numpy(policy_dist).float(),
            'value': torch.tensor(self.values[idx]).float()
        }
