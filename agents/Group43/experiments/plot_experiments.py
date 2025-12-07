import sys
import os
import matplotlib.pyplot as plt

def parse_log(filename):
    """
    Parses the new CSV-style output from Experiment.cpp.
    Returns: nps, lengths, rave_wins, total_games, tt_hits
    """
    nps = 0
    lengths = []
    tt_hits = []
    rave_wins = 0
    total_games = 0

    if not os.path.exists(filename):
        print(f"Error: File {filename} not found.")
        return 0, [], 0, 0, []

    print(f"Processing {filename}...")

    with open(filename, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    is_csv_section = False

    for line in lines:
        line = line.strip()

        # 1. Parse NPS (New format: "NPS Score: 12345")
        if "NPS Score:" in line:
            try:
                parts = line.split(':')
                nps = int(parts[1].strip())
            except ValueError:
                pass

        # 2. Detect CSV Section Start
        if line.startswith("GameID,RedPlayer"):
            is_csv_section = True
            continue

        # 3. Detect CSV Section End
        if is_csv_section and line.startswith("-"):
            is_csv_section = False
            continue

        # 4. Parse CSV Data
        # Format: GameID,RedPlayer,BluePlayer,Winner,Moves,TTHits
        if is_csv_section and line:
            try:
                parts = line.split(',')
                if len(parts) < 5: continue 

                # Extract Data
                red_player = parts[1]
                blue_player = parts[2]
                winner = parts[3]
                moves = int(parts[4])

                # Handle TT Hits (might be missing in baseline)
                hits = 0
                if len(parts) >= 6:
                    hits = int(parts[5])

                # Store Stats
                lengths.append(moves)
                tt_hits.append(hits)
                total_games += 1

                # Determine if RAVE won
                # RAVE wins if (Winner is Red AND Red is RAVE) OR (Winner is Blue AND Blue is RAVE)
                if (winner == "Red" and red_player == "RAVE") or \
                   (winner == "Blue" and blue_player == "RAVE"):
                    rave_wins += 1

            except ValueError:
                continue # Skip malformed lines

    return nps, lengths, rave_wins, total_games, tt_hits

def plot_uct_improvement(unopt_rave_wins, unopt_total, opt_rave_wins, opt_total, pool_rave_wins, pool_total):
    """
    Plots the win rate of UCT (The opponent of RAVE).
    Higher UCT win rate = Stronger Defense against RAVE.
    """
    # UCT Win Rate = 100% - RAVE Win Rate
    # We use 0 as default to avoid division by zero
    pool_uct_wr = 100 - (pool_rave_wins / pool_total * 100) if pool_total > 0 else 0
    opt_uct_wr = 100 - (opt_rave_wins / opt_total * 100) if opt_total > 0 else 0

    scenarios = ['Without TT', 'With TT']
    win_rates = [pool_uct_wr, opt_uct_wr]

    plt.figure(figsize=(8, 6))
    bars = plt.bar(scenarios, win_rates, color=['#95a5a6', '#3498db'])
    plt.title('Impact of Transposition Table on UCT Strength')
    plt.ylabel('UCT Win Rate (%)')
    plt.ylim(0, max(60, max(win_rates) + 10))

    # Add improvement annotation
    if opt_total > 0 and pool_total > 0:
        improvement = opt_uct_wr - pool_uct_wr
        plt.annotate('', xy=(1, opt_uct_wr), xytext=(0, pool_uct_wr),
                    arrowprops=dict(arrowstyle="->", color='black', lw=1.5))
        plt.text(0.5, (pool_uct_wr + opt_uct_wr)/2 + 2, 
                f'+{improvement:.1f}% Improvement', 
                ha='center', fontweight='bold', color='green')

    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height + 1,
                f'{height:.1f}%', ha='center', va='bottom', fontweight='bold')

    output_path = 'plots/tt_impact_uct_improvement.png'
    plt.savefig(output_path)
    print(f"Generated {output_path}")

def plot_nps(opt_nps, unopt_nps):
    configs = ['Baseline', 'Optimized']
    nps = [unopt_nps, opt_nps]

    plt.figure(figsize=(7, 5))
    bars = plt.bar(configs, nps, color=['#95a5a6', '#2ecc71'])
    plt.title('Search Speed Comparison (Nodes Per Second)')
    plt.ylabel('NPS')

    # Dynamic Y-axis
    max_val = max(nps) if nps else 100
    plt.ylim(0, max_val * 1.2)

    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{int(height):,}', ha='center', va='bottom', fontweight='bold')

    output_path = 'plots/nps_comparison.png'
    plt.savefig(output_path)
    print(f"Generated {output_path}")

def plot_tt_hits(tt_hits):
    if not tt_hits or sum(tt_hits) == 0:
        return

    plt.figure(figsize=(10, 5))
    games = range(1, len(tt_hits) + 1)
    plt.bar(games, tt_hits, color='#8e44ad', alpha=0.7)
    plt.title('Transposition Table Usage per Game')
    plt.xlabel('Game Number')
    plt.ylabel('Total Hash Hits')
    plt.grid(axis='y', linestyle='--', alpha=0.3)

    output_path = 'plots/tt_hits_per_game.png'
    plt.savefig(output_path)
    print(f"Generated {output_path}")

def plot_win_rate_pie(rave_wins, total_games, filename, title):
    if total_games == 0: return

    uct_wins = total_games - rave_wins
    labels = ['RAVE', 'UCT']
    sizes = [rave_wins, uct_wins]
    colors = ['#2ecc71', '#e74c3c']

    plt.figure(figsize=(6, 6))
    plt.pie(sizes, labels=labels, autopct='%1.1f%%', startangle=90, colors=colors)
    plt.title(f"{title}\n(Total Games: {total_games})")

    plt.savefig(filename)
    print(f"Generated {filename}")

if __name__ == "__main__":
    # Default file paths
    # You can pass arguments: python plot_experiments.py opt.csv unopt.csv nott.csv
    OPT_LOG = 'optimized_results.csv'
    UNOPT_LOG = 'baseline_results.csv'
    NOTT_LOG = 'nott_results.csv' # Optional intermediate log

    if len(sys.argv) > 1: OPT_LOG = sys.argv[1]
    if len(sys.argv) > 2: UNOPT_LOG = sys.argv[2]
    if len(sys.argv) > 3: NOTT_LOG = sys.argv[3]

    # 1. Parse Data
    opt_nps, opt_len, opt_rave, opt_total, opt_hits = parse_log(OPT_LOG)
    unopt_nps, unopt_len, unopt_rave, unopt_total, unopt_hits = parse_log(UNOPT_LOG)
    nott_nps, nott_len, nott_rave, nott_total, nott_hits = parse_log(NOTT_LOG)

    # Ensure output directory exists
    if not os.path.exists('plots'):
        os.makedirs('plots')

    # 2. Generate Plots

    # A. Search Speed (NPS)
    plot_nps(opt_nps, unopt_nps)

    # B. UCT Improvement (TT Impact)
    plot_uct_improvement(unopt_rave, unopt_total, opt_rave, opt_total, nott_rave, nott_total)

    # C. Transposition Table Hits
    plot_tt_hits(opt_hits)

    # D. Overall Win Rates (Pie Charts)
    plot_win_rate_pie(opt_rave, opt_total, 'plots/win_rate_optimized.png', 'Optimized Win Rate')
    plot_win_rate_pie(unopt_rave, unopt_total, 'plots/win_rate_baseline.png', 'Baseline Win Rate')

    print("\nDone! Check the 'plots/' directory for your images.")
