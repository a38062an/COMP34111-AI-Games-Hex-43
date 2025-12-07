#include "MCTS.h"
#include "Bitboard.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace std;

// ==========================================
// CONFIGURATION
// ==========================================
const int TIME_LIMIT_MS = 50;     // Time per move (Lower this for mass testing, e.g., 50ms)
const int NUM_GAMES = 20;         // Total games to run
const double RAVE_CONST = 1000.0; // RAVE parameter 'k'
const double UCT_CONST = 1.414;   // UCT parameter 'c'

// ==========================================
// BENCHMARKING (Nodes Per Second)
// ==========================================
void runNPSBenchmark()
{
    cout << "--------------------------------------" << endl;
    cout << "Running Speed Benchmark (NPS)..." << endl;

    // 1. Warmup (Get CPU cache hot)
    Bitboard board;
    MCTS mcts(board, 'R', 1.414, 0.0);
    mcts.runSearch(200); // Short warmup

    // 2. Actual Measure
    extern long long g_nodeCount;
    g_nodeCount = 0;

    auto start = chrono::high_resolution_clock::now();
    mcts.runSearch(1000); // Run for exactly 1 second
    auto end = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Time Elapsed:   " << duration << "ms" << endl;
    cout << "Nodes Expanded: " << g_nodeCount << endl;
    cout << "NPS Score:      " << (g_nodeCount * 1000) / duration << endl;
    cout << "--------------------------------------" << endl
         << endl;
}

// ==========================================
// EXPERIMENT RUNNER
// ==========================================
int main()
{
    // CRITICAL: Initialize lookup tables before anything else
    Bitboard::initTables();

    // 1. Run Speed Test
    runNPSBenchmark();

    // 2. Setup Tournament
    int raveWins = 0;
    int uctWins = 0;

    cout << "Starting Tournament: RAVE (" << RAVE_CONST << ") vs UCT (0.0)" << endl;
    cout << "Games: " << NUM_GAMES << " | Time/Move: " << TIME_LIMIT_MS << "ms" << endl;

#ifdef NO_POOL
    cout << "Build: UNOPTIMIZED (Standard new/delete)" << endl;
#else
    cout << "Build: OPTIMIZED (Memory Pool + TT)" << endl;
#endif

    cout << "--------------------------------------" << endl;
    // CSV Header for easy data processing later
    cout << "GameID,RedPlayer,BluePlayer,Winner,Moves,TTHits" << endl;

    for (int game = 1; game <= NUM_GAMES; ++game)
    {
// Reset Shared Resources
#ifndef NO_TT
        MCTS::tt.clear();
#endif

        Bitboard board;
        char turn = 'R';
        int moves = 0;

        // Swap roles every game to ensure fairness (Red advantage)
        // Odd Games: Red=RAVE, Blue=UCT
        // Even Games: Red=UCT, Blue=RAVE
        bool raveIsRed = (game % 2 != 0);
        string redName = raveIsRed ? "RAVE" : "UCT";
        string blueName = raveIsRed ? "UCT" : "RAVE";

        while (true)
        {
            // 1. Check Win
            if (board.checkWinRed())
            {
                (raveIsRed) ? raveWins++ : uctWins++;
                cout << game << "," << redName << "," << blueName << ",Red," << moves;
                break;
            }
            if (board.checkWinBlue())
            {
                (!raveIsRed) ? raveWins++ : uctWins++;
                cout << game << "," << redName << "," << blueName << ",Blue," << moves;
                break;
            }

            // 2. Determine Strategy for Current Player
            double currentRaveK = 0.0;
            if (turn == 'R')
            {
                currentRaveK = (redName == "RAVE") ? RAVE_CONST : 0.0;
            }
            else
            {
                currentRaveK = (blueName == "RAVE") ? RAVE_CONST : 0.0;
            }

            // 3. Run MCTS
            // Note: In a real game, we would reuse the tree.
            // Here we rebuild to test raw search power from scratch.
            MCTS mcts(board, turn, UCT_CONST, currentRaveK);
            pair<int, int> move = mcts.runSearch(TIME_LIMIT_MS);

            // 4. Handle Draw/No Moves (Should typically not happen in Hex)
            if (move.first == -1)
            {
                cout << game << "," << redName << "," << blueName << ",Draw," << moves;
                break;
            }

            // 5. Apply Move
            board.set(move.first, move.second, turn);
            turn = (turn == 'R') ? 'B' : 'R';
            moves++;
        }

#ifndef NO_TT
        cout << "," << MCTS::tt.ttHits << endl;
#else
        cout << ",0" << endl;
#endif
    }

    // ==========================================
    // FINAL REPORT
    // ==========================================
    cout << "--------------------------------------" << endl;
    cout << "FINAL RESULTS" << endl;
    cout << "--------------------------------------" << endl;
    cout << fixed << setprecision(2);
    cout << "RAVE Wins: " << raveWins << " (" << (double)raveWins / NUM_GAMES * 100.0 << "%)" << endl;
    cout << "UCT Wins:  " << uctWins << " (" << (double)uctWins / NUM_GAMES * 100.0 << "%)" << endl;

    return 0;
}