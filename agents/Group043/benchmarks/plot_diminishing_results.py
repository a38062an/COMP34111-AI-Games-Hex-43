import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

# Configure Path to find 'src' module if needed
project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

# UPDATED: Points to diminishing_returns.csv
RESULTS_FILE = os.path.join(project_root, "agents/Group43/results/diminishing_returns.csv")
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

    if df.empty:
        print("Dataset is empty. Skipping plots.")
        return

    # Calculate Win Rate (Winner == 1 means AggressiveAgent/TestAgent won)
    df['TestAgentWin'] = (df['Winner'] == 1).astype(int)

    # Group by TestTimeConfig
    grouped = df.groupby('TestTimeConfig').agg({
        'TestAgentWin': 'mean',
        'Score': 'mean',
        'TestAgentTime': 'mean',
        'BaselineAgentTime': 'mean',
        'TotalTurns': 'mean'
    }).reset_index()

    grouped['WinRatePercent'] = grouped['TestAgentWin'] * 100

    print("\nSummary Statistics (High Aggression):")
    print(grouped[['TestTimeConfig', 'WinRatePercent', 'Score', 'TestAgentTime', 'BaselineAgentTime']])

    # Plot 1: Win Rate vs Time Config
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['WinRatePercent'], marker='o', linestyle='-', color='purple')
    plt.title('Win Rate vs Time Limit (High Aggression 2.3x)')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Win Rate (%)')
    plt.grid(True)
    plt.ylim(-5, 105)
    output_path = os.path.join(PLOTS_DIR, "diminishing_win_rate.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

    # Plot 2: Average Score vs Time Config
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['Score'], marker='s', linestyle='-', color='orange')
    plt.title('Average Score vs Time Limit (High Aggression 2.3x)')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Average Score (Weighted Win + Speed)')
    plt.grid(True)
    output_path = os.path.join(PLOTS_DIR, "diminishing_score.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

    # Plot 3: Time Usage
    plt.figure(figsize=(10, 6))
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['TestAgentTime'], marker='o', label='High Aggression Agent', color='red')
    plt.plot(grouped['TestTimeConfig'] / 1000, grouped['BaselineAgentTime'], marker='x', linestyle='--', label='Baseline Agent', color='gray')
    plt.title('Actual Time Used vs Time Limit (High Aggression 2.3x)')
    plt.xlabel('Time Limit (seconds)')
    plt.ylabel('Average Time Used (seconds)')
    plt.legend()
    plt.grid(True)
    output_path = os.path.join(PLOTS_DIR, "diminishing_time_usage.png")
    plt.savefig(output_path)
    print(f"Saved plot to {output_path}")
    plt.close()

if __name__ == "__main__":
    plot_results()
