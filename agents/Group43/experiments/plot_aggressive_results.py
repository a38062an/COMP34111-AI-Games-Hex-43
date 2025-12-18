import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

# Configure Path to find 'src' module if needed, though not strictly necessary for plotting
# but good practice if we were importing project modules.
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

RESULTS_FILE = os.path.join(project_root, "agents/Group43/results/aggressive_returns.csv")
PLOTS_DIR = os.path.join(project_root, "agents/Group43/plots")

def ensure_dirs():
    os.makedirs(PLOTS_DIR, exist_ok=True)

def plot_results():
    if not os.path.exists(RESULTS_FILE):
        print(f"Error: Results file not found at {RESULTS_FILE}")
        return

    print(f"Reading data from {RESULTS_FILE}...")
    try:
        df = pd.read_csv(RESULTS_FILE)
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return

    ensure_dirs()

    # Calculate Win Rate (Winner == 1 means AggressiveAgent/TestAgent won)
    # 1 = Aggressive, 2 = Baseline
    df['TestAgentWin'] = (df['Winner'] == 1).astype(int)

    # Group by TestTimeConfig
    # We want to see how performance changes as we give the aggressive agent more time
    # Note: Baseline time is constant (likely 150000ms based on benchmark script)
    grouped = df.groupby('TestTimeConfig').agg({
        'TestAgentWin': 'mean',
        'Score': 'mean',
        'TestAgentTime': 'mean',
        'BaselineAgentTime': 'mean',
        'TotalTurns': 'mean'
    }).reset_index()

    grouped['WinRatePercent'] = grouped['TestAgentWin'] * 100

    print("\nSummary Statistics:")
    print(grouped[['TestTimeConfig', 'WinRatePercent', 'Score', 'TestAgentTime', 'BaselineAgentTime']])

    # Plot 1: Win Rate vs Time Config
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['WinRatePercent'], marker='o', linestyle='-', color='b')
    plt.title('Win Rate vs Time Limit')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Win Rate (%)')
    plt.grid(True)
    plt.ylim(-5, 105)
    output_path = os.path.join(PLOTS_DIR, "aggressive_win_rate.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

    # Plot 2: Average Score vs Time Config
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['Score'], marker='s', linestyle='-', color='g')
    plt.title('Average Score vs Time Limit')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Average Score (Weighted Win + Speed)')
    plt.grid(True)
    output_path = os.path.join(PLOTS_DIR, "aggressive_score.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

    # Plot 3: Time Usage
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['TestAgentTime'], marker='o', label='Aggressive Agent (Test)', color='r')
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['BaselineAgentTime'], marker='x', linestyle='--', label='Baseline Agent (Ref)', color='gray')
    plt.title('Actual Time Used vs Time Limit')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Average Time Used (seconds)')
    plt.legend()
    plt.grid(True)
    output_path = os.path.join(PLOTS_DIR, "aggressive_time_usage.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

if __name__ == "__main__":
    plot_results()
