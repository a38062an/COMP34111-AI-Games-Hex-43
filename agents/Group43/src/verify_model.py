import torch
import time
from src.model import HexModel

def verify():
    model = HexModel()
    
    # 1. Parameter Count
    params = sum(p.numel() for p in model.parameters())
    print(f"Total Parameters: {params:,}")
    if params > 150000:
        print(f"Parameter count exceeds 150k. Currently at: {params}")
    
    # 2. Inference Speed
    model.eval()
    dummy_input = torch.randn(1, 6, 11, 11)
    
    # Warmup
    for i in range(100):
        i = model(dummy_input)
        
    # Timing
    start = time.time()
    for i in range(1000):
        i = model(dummy_input)
    end = time.time()
    
    avg_latency = (end - start) / 1000 * 1000 # in ms
    print(f"Average Latency: {avg_latency:.4f} ms")

if __name__ == "__main__":
    verify()