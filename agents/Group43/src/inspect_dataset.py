import sys
import os
sys.path.append(os.getcwd())
from agents.Group43.src.dataset import HexNativeDataset
import matplotlib.pyplot as plt
import numpy as np
import torch

DATA_DIR = "agents/Group43/src/hex_dataset_raw/hex3_27x_b28.bin.gz/tdata"

def inspect():
    try:
        ds = HexNativeDataset(DATA_DIR)
        print(f"Dataset length: {len(ds)}")
        
        sample = ds[0]
        feat = sample['feature']
        pol = sample['policy']
        val = sample['value']
        
        print(f"Feature: {feat.shape}, Range: [{feat.min()}, {feat.max()}]")
        print(f"Policy: {pol.shape}, Sum: {pol.sum():.4f}")
        print(f"Value: {val}")
        
        # Viz
        # Plane 0: My Stones
        # Plane 1: Opp Stones
        
        my = feat[0].numpy()
        opp = feat[1].numpy()
        
        img = np.zeros((11, 11, 3))
        img[my==1] = [1, 0, 0] # Red
        img[opp==1] = [0, 0, 1] # Blue
        
        plt.figure(figsize=(5,5))
        plt.imshow(img)
        plt.title(f"Native Loader Test\nVal: {val:.2f}")
        plt.savefig("agents/Group43/src/Visualisations/native_loader_test.png")
        print("Variable check passed. Visualization saved.")
        
    except Exception as e:
        print(f"Loader failed: {e}")

if __name__ == "__main__":
    inspect()
