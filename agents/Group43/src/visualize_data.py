import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import RegularPolygon
import argparse
import os

def draw_hex_board(features, policy, value, save_path):
    """
    Draws a premium Hex board visualization.
    """
    # 1. Setup Data
    board_size = 11
    my_stones = features[0]   # 1 if my stone
    opp_stones = features[1]  # 1 if opp stone
    color_plane = features[5] # 1.0 if Red, 0.0 if Blue
    
    is_red_current = (color_plane[0,0] > 0.5)
    
    # Determine who is who
    # if is_red_current: My=Red, Opp=Blue
    # else: My=Blue, Opp=Red
    
    red_stones = my_stones if is_red_current else opp_stones
    blue_stones = opp_stones if is_red_current else my_stones
    
    # Policy move
    move_r = policy // 11
    move_c = policy % 11
    
    # 2. Setup Plot
    fig, ax = plt.subplots(figsize=(10, 10))
    ax.set_aspect('equal')
    
    # Background Colors
    # Red connects Top-Bottom? Usually Hex conventions:
    # Standard: Red/Black Start=Top/Bottom? Or Left/Right?
    # KataHex/Board.py: Red checks x=0 to x=size-1 (Rows? Top-Bottom).
    # Blue checks y=0 to y=size-1 (Cols? Left-Right).
    
    # Colors (
    COLOR_RED = '#E74C3C'    # Alizarin
    COLOR_BLUE = '#3498DB'   # Peter River
    COLOR_EMPTY = '#ECF0F1'  # Clouds
    COLOR_BG = '#FFFFFF'     # White
    COLOR_GRID = '#BDC3C7'   # Silver
    COLOR_HIGHLIGHT = '#2ECC71' # Emerald (Green)
    
    # 3. Draw Hexagons
    # Coordinate Transform: (r, c) -> (x, y)
    # x = c + r * 0.5
    # y = -r * sqrt(3)/2  (Negative y to put row 0 at top)
    
    hex_radius = 0.55 # Slightly overlapping or touching? distance center-corner?
    # Distance center-to-center is 1.0 horizontally. 
    # Height of row is sqrt(3)/2 ~= 0.866.
    # Radius (center to vertex) should be 1/sqrt(3) ~= 0.577 for tight packing.
    radius = 0.577
    
    for r in range(board_size):
        for c in range(board_size):
            x = c + r * 0.5
            y = -r * (np.sqrt(3) / 2)
            
            # Determine Color
            face_color = COLOR_EMPTY
            alpha = 1.0
            edge_color = COLOR_GRID
            lw = 1.5
            
            if red_stones[r, c] == 1:
                face_color = COLOR_RED
                edge_color = '#C0392B'
            elif blue_stones[r, c] == 1:
                face_color = COLOR_BLUE
                edge_color = '#2980B9'
                
            # Highlight Move
            if r == move_r and c == move_c:
                edge_color = COLOR_HIGHLIGHT
                lw = 4.0
                # If empty, maybe fill with light green?
                if face_color == COLOR_EMPTY:
                    face_color = '#D5F5E3' # Light Green
            
            hex_patch = RegularPolygon(
                (x, y), 
                numVertices=6, 
                radius=radius, 
                orientation=np.radians(30), # Pointy top
                facecolor=face_color, 
                edgecolor=edge_color,
                linewidth=lw,
                alpha=alpha
            )
            ax.add_patch(hex_patch)
            
            # Coordinate Label (Optional, subtle)
            # ax.text(x, y, f"{r},{c}", ha='center', va='center', fontsize=6, color='#7F8C8D')

    # 4. Add Border/Labels
    # Approximate bounds
    min_x = 0
    max_x = 10 + 10 * 0.5
    min_y = -10 * (np.sqrt(3)/2)
    max_y = 0
    
    pad = 1.0
    ax.set_xlim(min_x - pad, max_x + pad)
    ax.set_ylim(min_y - pad, max_y + pad)
    
    # Hide axis
    ax.set_axis_off()
    
    # 5. Info Text
    current_player_name = "Red" if is_red_current else "Blue"
    current_player_color = COLOR_RED if is_red_current else COLOR_BLUE
    
    # Title
    plt.title(f"Processed Training Sample", fontsize=20, fontweight='bold', pad=20, color='#2C3E50')
    
    # Annotations
    info_x = min_x
    info_y = max_y + 1.0
    
    # Value
    win_prob = (value + 1) / 2 * 100 # -1..1 -> 0..100
    
    plt.text(min_x, min_y - 1.5, f"Current Player: {current_player_name}", 
             fontsize=14, color=current_player_color, fontweight='bold')
    
    plt.text(min_x, min_y - 2.2, f"Target Value (Win%): {value:.2f}", 
             fontsize=14, color='#34495E')
             
    plt.text(min_x, min_y - 2.9, f"Move Played: ({move_r}, {move_c})", 
             fontsize=14, color=COLOR_HIGHLIGHT, fontweight='bold')

    # Save
    os.makedirs(os.path.dirname(save_path), exist_ok=True)
    plt.savefig(save_path, bbox_inches='tight', dpi=150)
    print(f"Saved modern visualization to: {save_path}")
    plt.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--file', type=str, default='agents/Group43/src/processed_data/dataset.npz')
    parser.add_argument('--num', type=int, default=3)
    parser.add_argument('--out_dir', type=str, default='agents/Group43/src/visualizations')
    args = parser.parse_args()
    
    if not os.path.exists(args.file):
        print(f"File not found: {args.file}")
        exit(1)
        
    print(f"Loading {args.file}...")
    data = np.load(args.file)
    X = data['features']
    y_pol = data['policies']
    y_val = data['values']
    
    total = len(X)
    indices = np.random.choice(total, args.num, replace=False)
    
    for idx in indices:
        save_path = os.path.join(args.out_dir, f"sample_{idx}.png")
        draw_hex_board(X[idx], y_pol[idx], y_val[idx], save_path)
