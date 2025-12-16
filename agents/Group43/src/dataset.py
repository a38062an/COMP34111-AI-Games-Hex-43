import torch
from torch.utils.data import Dataset
import numpy as np
import os

class HexDataset(Dataset):
    def __init__(self, data_path, split='train', val_split=0.1, seed=42):
        """
        Args:
            data_path (str): Path to the .npz file containing 'features', 'policies', 'values'.
            split (str): 'train' or 'val'.
            val_split (float): Fraction of data to use for validation.
            seed (int): Random seed for splitting accuracy.
        """
        if not os.path.exists(data_path):
            raise FileNotFoundError(f"Data file not found: {data_path}")
            
        print(f"Loading dataset from {data_path}...")
        with np.load(data_path) as data:
            self.features = data['features']
            self.policies = data['policies']
            self.values = data['values']
            
        total_len = len(self.features)
        indices = np.arange(total_len)
        
        # Shuffle for split
        np.random.seed(seed)
        np.random.shuffle(indices)
        
        split_idx = int(total_len * (1 - val_split))
        
        if split == 'train':
            self.indices = indices[:split_idx]
        else: # val
            self.indices = indices[split_idx:]
            
        print(f"Dataset loaded. Split: {split}. Samples: {len(self.indices)}/{total_len}")

    def __len__(self):
        return len(self.indices)

    def __getitem__(self, idx):
        # Map logical index to physical index
        real_idx = self.indices[idx]
        
        # Load
        feature = self.features[real_idx] # (6, 11, 11)
        policy = self.policies[real_idx]  # scalar (0-120)
        value = self.values[real_idx]     # scalar (-1 or 1)
        
        # Convert to Tensor
        return {
            'feature': torch.from_numpy(feature).float(),
            'policy': torch.tensor(policy).long(),
            'value': torch.tensor(value).float()
        }

if __name__ == "__main__":
    # Test
    path = "agents/Group43/src/processed_data/dataset.npz"
    if os.path.exists(path):
        ds = HexDataset(path, split='train')
        print("Sample 0 Feature Shape:", ds[0]['feature'].shape)
        print("Sample 0 Policy:", ds[0]['policy'])
        print("Sample 0 Value:", ds[0]['value'])
