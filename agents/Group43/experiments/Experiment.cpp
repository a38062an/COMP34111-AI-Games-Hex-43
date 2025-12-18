#include "../src/MCTS.h"
#include "../src/Bitboard.h"
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace std;

// This global variable must exist in your MCTS.cpp (as seen in your previous code)
extern long long g_nodeCount;

int main()
{
    // 1. Critical Setup
    Bitboard::initTables();
    cout << "===========================================" << endl;
    cout << "      Hex Agent Speed Benchmark (NPS)      " << endl;
    cout << "===========================================" << endl;

    // 2. Setup Board and Agent
    Bitboard board; // Empty Board
    
    // Load Model
    torch::jit::script::Module module;
    try {
        module = torch::jit::load("src/checkpoints/hex_model.pt");
        module.eval();
        cout << "Model loaded successfully." << endl;
    } catch (const c10::Error& e) {
        cerr << "Error loading model: " << e.msg() << endl;
        return -1;
    }

    // Parameters: 'R' (Red), Model, C=1.414, RAVE_K=1000
    MCTS mcts(board, 'R', &module, 1.414, 1000.0);

    // 3. Warmup Phase
    // (Runs for 500ms to ensure caches are hot and memory pool is active)
    cout << "1. Warming up (500ms)..." << flush;
    mcts.runSearch(500); 
    cout << " Done." << endl;

    // 4. Reset Counters
    g_nodeCount = 0;
    int benchmarkDurationMs = 5000; // Run for 5 seconds for stability

    // 5. The Benchmark
    cout << "2. Running Stress Test (" << benchmarkDurationMs / 1000 << "s)..." << endl;
    
    auto start = chrono::high_resolution_clock::now();
    mcts.runSearch(benchmarkDurationMs);
    auto end = chrono::high_resolution_clock::now();

    // 6. Calculate Results
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double seconds = duration / 1000.0;
    long long nps = (long long)(g_nodeCount / seconds);

    cout << "-------------------------------------------" << endl;
    cout << "Total Simulations: " << g_nodeCount << endl;
    cout << "Total Time:        " << seconds << " s" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "Simulations/Sec:   " << nps << endl;
    cout << "-------------------------------------------" << endl;

    return 0;
}