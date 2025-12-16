import os
import glob
import numpy as np
import re
import argparse
from tqdm import tqdm
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

from src.Board import Board
from src.Colour import Colour


def parse_sgf_coord(coord_str):
    """Converts SGF coordinate (e.g. 'a1', 'k11') to (row, col)."""
    if not coord_str or len(coord_str) < 2:
        return None
    
    col_char = coord_str[0].lower()
    row_str = coord_str[1:]
    
    # Map a->0, b->1, ... k->10
    if not 'a' <= col_char <= 'z':
        return None
        
    col = ord(col_char) - ord('a')
    try:
        row = int(row_str) - 1
    except ValueError:
        return None
        
    return row, col

def board_to_features(board, color):
    """
    Generates 6-plane feature tensor from board state.
    Planes:
    0: My stones
    1: Opponent stones
    2: Empty points
    3: My Edge Distance (Y/X) - Normalized coordinates
    4: Opp Edge Distance (X/Y)
    5: Color (1 for Red, 0 for Blue)
    
    Returns: (6, 11, 11) float32 numpy array
    """
    size = board.size
    features = np.zeros((6, size, size), dtype=np.float32)
    
    my_colour = color
    opp_colour = Colour.BLUE if color == Colour.RED else Colour.RED
    
    # Efficiently access tiles
    # Board.tiles is list[list[Tile]], indexed [row][col]
    for r in range(size):
        for c in range(size):
            tile_col = board.tiles[r][c].colour
            if tile_col == my_colour:
                features[0, r, c] = 1.0
            elif tile_col == opp_colour:
                features[1, r, c] = 1.0
            else:
                features[2, r, c] = 1.0
                
    # Coordinate grids for distance planes
    # Normalized 0..1
    y_coords, x_coords = np.mgrid[0:size, 0:size]
    y_norm = y_coords / (size - 1)
    x_norm = x_coords / (size - 1)
    
    if my_colour == Colour.RED:
        # Red connects Top-Bottom (Rows). Valid moves move 'vertical' ish.
        # Use row index as primary distance proxy.
        features[3] = y_norm
        features[4] = x_norm
        features[5] = 1.0 # Red plane
    else:
        # Blue connects Left-Right (Cols).
        features[3] = x_norm # My distance is along X
        features[4] = y_norm
        features[5] = 0.0 # Blue plane
        
    return features

def process_sgf_file(filepath, board_size=11):
    """Reads an SGF file and yields inputs, policies, values."""
    inputs = []
    policies = []
    values = []
    
    with open(filepath, 'r') as f:
        content = f.read()
        
    # Split games. Usually each line is a game or separated by )
    # Our file has one game per line.
    games = content.strip().split('\n')
    
    valid_games = 0
    
    for game_str in tqdm(games, desc=f"Processing {os.path.basename(filepath)}", leave=False):
        if not game_str.strip() or "(;" not in game_str:
            continue
            
        board = Board(board_size)
        
        # 1. Parse Winner
        # RE[B...] or RE[W...]
        res_match = re.search(r'RE\[([BW])', game_str)
        if not res_match:
            continue # Skip games with no result
        winner_char = res_match.group(1)
        winner_colour = Colour.RED if winner_char == 'B' else Colour.BLUE
        
        # 2. Setup Stones
        # AB[...] AW[...]
        def apply_setup(text, c):
            coords = re.findall(r'\[([a-zA-Z][0-9]+)\]', text)
            for coord in coords:
                rc = parse_sgf_coord(coord)
                if rc:
                    board.set_tile_colour(rc[0], rc[1], c)

        ab_tags = re.findall(r'AB((?:\[[^\]]*\])+)', game_str)
        for tag in ab_tags: apply_setup(tag, Colour.RED)
            
        aw_tags = re.findall(r'AW((?:\[[^\]]*\])+)', game_str)
        for tag in aw_tags: apply_setup(tag, Colour.BLUE)
        
        # 3. Parse Moves
        # ;B[coord] or ;W[coord]
        # We assume they are in order.
        raw_moves = re.findall(r';([BW])\[([a-zA-Z][0-9]{1,2})\]', game_str)
        
        for player_char, coord_str in raw_moves:
            rc = parse_sgf_coord(coord_str)
            if not rc: continue
            r, c = rc
            
            if r >= board_size or c >= board_size: continue
            
            current_color = Colour.RED if player_char == 'B' else Colour.BLUE
            
            # --- Generate Data Point (BEFORE move is made) ---
            
            # Input: Board state viewed by current player
            feat = board_to_features(board, current_color)
            
            # Policy Target: The move they actually played
            move_idx = r * board_size + c
            
            # Value Target: Did they win eventually?
            # 1.0 if they won, -1.0 if lost
            val = 1.0 if current_color == winner_colour else -1.0
            
            inputs.append(feat)
            policies.append(move_idx)
            values.append(val)
            
            # --- Apply Move ---
            board.set_tile_colour(r, c, current_color)
            
        valid_games += 1
        
    return inputs, policies, values

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--sgf_dir', type=str, default='agents/Group43/src/hex_dataset_raw/seed_model.bin.gz/sgfs')
    parser.add_argument('--output_dir', type=str, default='agents/Group43/src/processed_data')
    args = parser.parse_args()
    
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Matching .sgfs files
    files = glob.glob(os.path.join(args.sgf_dir, '*.sgfs'))
    if not files:
        print(f"No .sgfs files found in {args.sgf_dir}")
        sys.exit(0)
        
    print(f"Found {len(files)} files. Starting processing...")
    
    all_inputs = []
    all_policies = []
    all_values = []
    
    for f in files:
        i, p, v = process_sgf_file(f)
        all_inputs.extend(i)
        all_policies.extend(p)
        all_values.extend(v)
        
    if not all_inputs:
        print("No data extracted.")
        sys.exit(0)
        
    # Convert to Numpy
    X = np.array(all_inputs, dtype=np.float32)
    y_pol = np.array(all_policies, dtype=np.int64)
    y_val = np.array(all_values, dtype=np.float32)
    
    print("-" * 30)
    print(f"Processing Complete.")
    print(f"Total Samples: {len(X)}")
    print(f"Features Shape: {X.shape}")
    print(f"Policy Shape:   {y_pol.shape}")
    print(f"Value Shape:    {y_val.shape}")
    
    save_path = os.path.join(args.output_dir, "dataset.npz")
    np.savez_compressed(save_path, features=X, policies=y_pol, values=y_val)
    print(f"Saved to: {save_path}")