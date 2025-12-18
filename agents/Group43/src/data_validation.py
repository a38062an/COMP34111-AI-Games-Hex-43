import re
import glob
import os
import sys
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

def analyze_sgf_quality(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Regex to find the comments with stats
    # Looks for C[... v=123 ...] pattern
    # The pattern matches: float float float ... v=Integer
    stat_pattern = re.compile(r'C\[([\d\.]+) ([\d\.]+) ([\d\.]+) .*?v=(\d+)')
    
    visits = []
    win_rates = []
    
    matches = stat_pattern.findall(content)
    
    for match in matches:
        win_prob = float(match[0]) # The first number is usually Win %
        visit_count = int(match[3]) # The number after v=
        
        visits.append(visit_count)
        win_rates.append(win_prob)

    return visits, win_rates

def batch_analyze(directory):
    total_games = 0
    all_visits = []
    blunder_count = 0
    
    # Process all .sgf files in the folder
    print(directory)
    files = glob.glob(os.path.join(directory, "*.sgfs"))
    
    print(f"Analyzing {len(files)} SGF files...")
    
    for filepath in files:
        game_visits, game_winrates = analyze_sgf_quality(filepath)
        
        if not game_visits:
            continue
            
        total_games += 1
        all_visits.extend(game_visits)
        
        # Check for blunders: A drop of > 40% win rate in one move
        for i in range(1, len(game_winrates)):
            # Note: Win rate perspective flips every turn in some logs, 
            # but usually usually represented as "Root player winrate". 
            # If standard SGF, we look for wild variance.
            diff = abs(game_winrates[i] - game_winrates[i-1])
            if diff > 0.4: 
                blunder_count += 1

    if total_games == 0:
        print("No valid data found.")
        return

    avg_visits = sum(all_visits) / len(all_visits)
    print("-" * 30)
    print(f"Data Quality Report")
    print("-" * 30)
    print(f"Total Moves Analyzed: {len(all_visits)}")
    print(f"Average Visits/Move:  {avg_visits:.1f}")
    print(f"Max Visits Seen:      {max(all_visits)}")
    print(f"Potential Blunders:   {blunder_count} (High swings in win-rate)")
    
    # Interpretation
    print("-" * 30)
    if avg_visits < 200:
        print("VERDICT: Low Playout Data (Speed/Bootstrap).")
        print("Good for: Initial training, learning basics.")
        print("Bad for: Learning complex tactical play.")
    elif avg_visits > 800:
        print("VERDICT: High Playout Data.")
        print("Good for: Fine-tuning, high-level strategy.")

# Usage: Replace '.' with your folder path containing the .sgf files
batch_analyze('agents/Group43/src/hex_dataset_raw/seed_model.bin.gz/sgfs/')