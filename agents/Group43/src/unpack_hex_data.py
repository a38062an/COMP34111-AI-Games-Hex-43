import numpy as np
import matplotlib.pyplot as plt
import os

# Target file to inspect
FILE_PATH = "agents/Group43/src/hex_dataset_raw/hex3_27x_b28.bin.gz/tdata/046131365EA6EB24.npz"

def unpack_bits(packed_data, board_size=11):
    """
    Unpacks binaryInputNCHWPacked from (N, C, 16) uint8 to (N, C, 11, 11) float.
    
    Args:
        packed_data: (batch, channels, 16) uint8 array.
        board_size: 11
        
    Returns:
        unpacked: (batch, channels, 11, 11) float32
    """
    N, C, B_packed = packed_data.shape
    # 16 bytes = 128 bits. We need 121 bits.
    
    # Expand to bits
    # np.unpackbits converts uint8 0xFF -> 11111111 (big endian view usually)
    # KataGo/Hex packs board scan order.
    
    bits = np.unpackbits(packed_data, axis=2) # (N, C, 128)
    
    # Trim to 121 bits
    bits = bits[:, :, :board_size*board_size] # (N, C, 121)
    
    # Reshape to (N, C, 11, 11)
    return bits.reshape(N, C, board_size, board_size).astype(np.float32)

def inspect_channels(file_path):
    if not os.path.exists(file_path):
        print("File not found.")
        return

    data = np.load(file_path)
    if 'binaryInputNCHWPacked' not in data:
        print("Key binaryInputNCHWPacked not found!")
        return
        
    packed = data['binaryInputNCHWPacked']
    print(f"Packed shape: {packed.shape}")
    
    unpacked = unpack_bits(packed)
    print(f"Unpacked shape: {unpacked.shape}")
    
    # Visualize Channel 0 and 1 for the first 3 samples
    # Typically Ch0 = Current Player Stones, Ch1 = Opponent Stones?
    # Or Ch0=Black, Ch1=White?
    
    cols = data['globalInputNC']
    # If using 'globalInputNC', it often has color info.
    
    samples_to_show = 3
    num_channels_to_show = 4 
    
    fig, axes = plt.subplots(samples_to_show, num_channels_to_show, figsize=(12, 10))
    
    for i in range(samples_to_show):
        for c in range(num_channels_to_show):
            ax = axes[i, c]
            ax.imshow(unpacked[i, c], cmap='Greys', vmin=0, vmax=1)
            ax.set_xticks([])
            ax.set_yticks([])
            if i == 0:
                ax.set_title(f"Channel {c}")
            if c == 0:
                ax.set_ylabel(f"Sample {i}")
                
    save_path = "agents/Group43/src/Visualisations/channel_inspection.png"
    plt.tight_layout()
    plt.savefig(save_path)
    print(f"Saved channel inspection to {save_path}")

if __name__ == "__main__":
    inspect_channels(FILE_PATH)
