#include "MCTS.h"
#include "Bitboard.h"
#include <iostream>

using namespace std;

// Config
const int TIME_LIMIT_MS = 4000; // 4.0s per turn
const int NUM_GAMES = 100; // 100 games for massive data collection

void runBenchmark() 
{
    cout << "Running Benchmark (1000ms)..." << endl;
    Bitboard board;
    // Use UCT (rave=0) for consistent benchmark
    MCTS mcts(board, 'R', 1.414, 0.0);
    
    // Run search for exactly 1000ms
    extern long long g_nodeCount;
    g_nodeCount = 0;
    
    auto start = chrono::high_resolution_clock::now();
    mcts.runSearch(1000);
    auto end = chrono::high_resolution_clock::now();
    
    cout << "Benchmark Complete." << endl;
    cout << "Nodes Expanded: " << g_nodeCount << endl;
    cout << "Nodes Per Second (NPS): " << g_nodeCount << endl;
}

int main() {
    runBenchmark();
    
    int raveWins = 0;
    int uctWins = 0;

    cout << "Starting Experiment: RAVE vs UCT" << endl;
#ifdef NO_POOL
    cout << "Configuration: UNOPTIMIZED (No Pool, No TT)" << endl;
#else
    cout << "Configuration: OPTIMIZED (Pool, TT, Zobrist)" << endl;
#endif
    cout << "Games: " << NUM_GAMES << ", Time Limit: " << TIME_LIMIT_MS << "ms" << endl;

    for (int game = 1; game <= NUM_GAMES; ++game) {
        // Clear TT to prevent sharing info between games/agents
#ifndef NO_TT
        MCTS::tt.clear();
#endif
        Bitboard board;
        char turn = 'R';
        int moves = 0;
        
        // Alternate who is RAVE
        // Odd games: RAVE is Red, UCT is Blue
        // Even games: UCT is Red, RAVE is Blue
        bool raveIsRed = (game % 2 != 0);
        
        cout << "Game " << game << ": " << (raveIsRed ? "RAVE(Red) vs UCT(Blue)" : "UCT(Red) vs RAVE(Blue)") << "... ";
        cout.flush();

        while (true) {
            if (board.checkWinRed()) {
                if (raveIsRed) raveWins++; else uctWins++;
                cout << "Red won";
                break;
            }
            if (board.checkWinBlue()) {
                if (!raveIsRed) raveWins++; else uctWins++;
                cout << "Blue won";
                break;
            }

            // Create MCTS
            // Determine if current player is RAVE or UCT
            double raveK = 0.0;
            if (raveIsRed) 
            {
                raveK = (turn == 'R') ? 1000.0 : 0.0;
            }
            else 
            {
                raveK = (turn == 'B') ? 1000.0 : 0.0;
            }
            
            MCTS mcts(board, turn, 1.414, raveK);
            
            // Run search
            pair<int, int> move = mcts.runSearch(TIME_LIMIT_MS);
            
            if (move.first == -1) {
                break;
            }

            board.set(move.first, move.second, turn);
            turn = (turn == 'R') ? 'B' : 'R';
            moves++;
        }
        
        cout << " in " << moves << " moves." << endl;
#ifndef NO_TT
        cout << "  TT Hits: " << MCTS::tt.ttHits << endl;
#endif
    }

    cout << "Final Results:" << endl;
    cout << "RAVE Wins: " << raveWins << " (" << (raveWins * 100.0 / NUM_GAMES) << "%)" << endl;
    cout << "UCT Wins:  " << uctWins << " (" << (uctWins * 100.0 / NUM_GAMES) << "%)" << endl;

    return 0;
}
