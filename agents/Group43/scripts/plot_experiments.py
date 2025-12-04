import matplotlib.pyplot as plt
import numpy as np
import sys
import re
import os

def parse_log(filename):
    nps = 0
    lengths = []
    rave_wins = 0
    total_games = 0
    
    if not os.path.exists(filename):
        print(f"Error: File {filename} not found.")
        return 0, [], 0, 0

    with open(filename, 'r') as f:
        content = f.read()
        
        # Parse NPS
        # "Nodes Per Second (NPS): 41231"
        nps_match = re.search(r"Nodes Per Second \(NPS\): (\d+)", content)
        if nps_match:
            nps = int(nps_match.group(1))
            
        # Parse Game Lengths
        # "Game 1: ... won in 35 moves."
        lengths = [int(m) for m in re.findall(r"won in (\d+) moves", content)]
        
        # Parse Win Rate (Robust for partial logs)
        # Check for "Final Results" first
        win_match = re.search(r"RAVE Wins: (\d+)", content)
        if win_match:
            rave_wins = int(win_match.group(1))
            total_games = len(lengths) # Or parse "Games: 20"
        else:
            # Fallback: Count wins manually from game lines
            # "Game 1: RAVE(Red) vs UCT(Blue)... Blue won" -> UCT Win
            # "Game 2: UCT(Red) vs RAVE(Blue)... Blue won" -> RAVE Win
            # RAVE is Red in odd games (1, 3...), Blue in even games (2, 4...)
            
            # Find all game lines
            game_lines = re.findall(r"Game (\d+): .*? (Red|Blue) won", content)
            rave_wins = 0
            total_games = 0
            
            for g_num, winner in game_lines:
                game_idx = int(g_num)
                total_games += 1
                
                rave_is_red = (game_idx % 2 != 0)
                
                if rave_is_red and winner == 'Red':
                    rave_wins += 1
                elif not rave_is_red and winner == 'Blue':
                    rave_wins += 1
        
    return nps, lengths, rave_wins, total_games

def plot_uct_improvement(unopt_rave_wins, unopt_total, opt_rave_wins, opt_total, pool_rave_wins, pool_total):
    # Calculate UCT Win Rates (100 - RAVE%)
    # We compare Pool Only (Baseline) vs Optimized (With TT)
    if pool_total > 0 and opt_total > 0:
        pool_uct_wr = 100 - (pool_rave_wins / pool_total * 100)
        opt_uct_wr = 100 - (opt_rave_wins / opt_total * 100)
        
        scenarios = ['Without TT', 'With TT']
        win_rates = [pool_uct_wr, opt_uct_wr]
        
        plt.figure(figsize=(8, 6))
        bars = plt.bar(scenarios, win_rates, color=['#95a5a6', '#3498db'])
        plt.title('Impact of Transposition Table on UCT Strength')
        plt.ylabel('UCT Win Rate (%)')
        plt.ylim(0, 60)
        
        # Add improvement arrow
        plt.annotate('', xy=(1, opt_uct_wr), xytext=(0, pool_uct_wr),
                    arrowprops=dict(arrowstyle="->", color='black', lw=1.5))
        plt.text(0.5, (pool_uct_wr + opt_uct_wr)/2 + 2, 
                f'+{opt_uct_wr - pool_uct_wr:.1f}% Improvement', 
                ha='center', fontweight='bold', color='green')
        
        for bar in bars:
            height = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2., height + 1,
                    f'{height:.1f}%', ha='center', va='bottom', fontweight='bold')
        
        output_path = 'plots/tt_impact_uct_improvement.png'
        plt.savefig(output_path)
        print(f"Generated {output_path}")

    # 2. Individual Plots (Keep these for reference)
    plot_single_win_rate(unopt_rave_wins, unopt_total, 'Baseline (Unoptimized)', 'plots/win_rate_baseline_unoptimized.png', '#95a5a6')
    plot_single_win_rate(opt_rave_wins, opt_total, 'Fully Optimized (TT + Pool)', 'plots/win_rate_fully_optimized.png', '#2ecc71')
    plot_single_win_rate(pool_rave_wins, pool_total, 'Pool Only (No TT)', 'plots/win_rate_pool_only.png', '#f39c12')

def plot_single_win_rate(rave_wins, total, title, filename, color):
    if total == 0: return
    
    rave_pct = (rave_wins / total) * 100
    uct_pct = 100 - rave_pct
    
    plt.figure(figsize=(6, 5))
    plt.bar(['RAVE', 'UCT'], [rave_pct, uct_pct], color=[color, '#34495e'])
    plt.title(f'Win Rate: {title}\n({total} Games)')
    plt.ylabel('Win Rate (%)')
    plt.ylim(0, 100)
    
    for i, v in enumerate([rave_pct, uct_pct]):
        plt.text(i, v + 2, f'{v:.1f}%', ha='center', fontweight='bold')
        
    plt.savefig(filename)
    print(f"Generated {filename}")

def plot_nps(opt_nps, unopt_nps):
    configs = ['Unoptimized', 'Optimized']
    nps = [unopt_nps, opt_nps]
    
    plt.figure(figsize=(6, 4))
    bars = plt.bar(configs, nps, color=['#95a5a6', '#2ecc71'])
    plt.title('Search Speed Comparison')
    plt.ylabel('Nodes Per Second (NPS)')
    
    min_nps = min(nps) if nps else 0
    max_nps = max(nps) if nps else 0
    plt.ylim(min_nps * 0.8, max_nps * 1.1)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:,}', ha='center', va='bottom', fontweight='bold')
    
    output_path = 'plots/performance_nps_comparison.png'
    plt.savefig(output_path)
    print(f"Generated {output_path}")

def plot_consistency(opt_lengths, unopt_lengths):
    plt.figure(figsize=(8, 5))
    data = [unopt_lengths, opt_lengths]
    plt.boxplot(data, tick_labels=['Unoptimized', 'Optimized'])
    plt.title('Game Length Consistency Distribution')
    plt.ylabel('Moves per Game')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    output_path = 'plots/game_length_consistency_distribution.png'
    plt.savefig(output_path)
    print(f"Generated {output_path}")

if __name__ == "__main__":
    # Updated paths to results/ directory
    opt_log = 'results/logs_optimized.txt'
    unopt_log = 'results/logs_unoptimized.txt'
    nott_log = 'results/logs_pool_only.txt'
    
    if len(sys.argv) > 1: opt_log = sys.argv[1]
    if len(sys.argv) > 2: unopt_log = sys.argv[2]
    if len(sys.argv) > 3: nott_log = sys.argv[3]
    
    print(f"Parsing Optimized Log: {opt_log}")
    opt_nps, opt_lengths, opt_rave_wins, opt_total = parse_log(opt_log)
    
    print(f"Parsing Unoptimized Log: {unopt_log}")
    unopt_nps, unopt_lengths, unopt_rave_wins, unopt_total = parse_log(unopt_log)
    
    print(f"Parsing Pool Only Log: {nott_log}")
    nott_nps, nott_lengths, nott_rave_wins, nott_total = parse_log(nott_log)

    # Ensure plots directory exists
    if not os.path.exists('plots'):
        os.makedirs('plots')

    # 1. Win Rates (All Scenarios)
    plot_uct_improvement(unopt_rave_wins, unopt_total, opt_rave_wins, opt_total, nott_rave_wins, nott_total)
    
    # 2. NPS Comparison
    plot_nps(opt_nps, unopt_nps)
    
    # 3. Consistency Comparison (Removed as per user request)
    # plot_consistency(opt_lengths, unopt_lengths)

    # 4. TT Hits Visualization
    with open(opt_log, 'r') as f:
        content = f.read()
        hits = [int(m) for m in re.findall(r"TT Hits: (\d+)", content)]
        
        if hits:
            plt.figure(figsize=(10, 5))
            games = range(1, len(hits) + 1)
            plt.bar(games, hits, color='#8e44ad')
            plt.title('Transposition Table Hits per Game')
            plt.xlabel('Game Number')
            plt.ylabel('Number of Hits')
            plt.tight_layout()
            output_path = 'plots/tt_hits_per_game_optimized.png'
            plt.savefig(output_path)
            print(f"Generated {output_path}")
