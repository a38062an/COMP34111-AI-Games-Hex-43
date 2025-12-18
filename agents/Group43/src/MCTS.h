#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <memory>
#include <algorithm>
#include <chrono>
#include <deque>
#include "Bitboard.h"
#include <torch/script.h>

#include "Node.h"

using namespace std;

/**
 * @brief Monte Carlo Tree Search (MCTS) engine.
 *
 * Implements the standard MCTS algorithm with 4 phases:
 * Selection, Expansion, Simulation, and Backpropagation.
 */
class MCTS
{
public:
    Bitboard rootBoard; ///< The board state at the root of the search tree
    char myColour;      ///< The colour of the agent running the search
    double explorationConstant;
    double raveConstant;

    torch::jit::script::Module* module; /// Pointer to loaded neural network
    
    /**
     * @brief Construct a new MCTS engine.
     *
     * @param board The current board state.
     * @param colour The agent's colour.
     * @param net Pointer to the neural network module (can be nullptr).
     * @param exploration UCT exploration constant (default 1.414).
     * @param rave RAVE constant (default 1000.0).
     */
    MCTS(const Bitboard &board, char colour, torch::jit::script::Module* net = nullptr, double exploration = 1.414, double rave = 1000.0)
        : rootBoard{board}, 
        myColour{colour}, 
        explorationConstant{exploration}, 
        raveConstant{rave},
        module{net}
    {
    }

    /**
     * @brief Run the MCTS search for a specified time duration.
     *
     * @param timeLimitMs Time limit in milliseconds.
     * @return pair<int, int> The best move coordinates (col, row).
     */
    struct SearchResult
    {
        pair<int, int> move;
        int iterations;
    };

    /**
     * @brief Run the MCTS search for a specified time duration.
     *
     * @param timeLimitMs Time limit in milliseconds.
     * @return SearchResult Best move and simulation count.
     */
    SearchResult runSearch(int timeLimitMs);

    struct TTEntry
    {
        uint64_t hash;
        double wins;
        int visits;
    };

    struct TranspositionTable
    {
        static const int SIZE = 1 << 20; // 1M entries
        vector<TTEntry> table;
        long long ttHits = 0;

        TranspositionTable() : table(SIZE) {}

        void store(uint64_t hash, double wins, int visits)
        {
            int index = hash % SIZE;
            // Simple replacement strategy: replace if more visits
            if (visits > table[index].visits)
            {
                table[index] = {hash, wins, visits};
            }
        }

        void clear()
        {
            fill(table.begin(), table.end(), TTEntry{0, 0.0, 0});
            ttHits = 0;
        }

        bool lookup(uint64_t hash, double &wins, int &visits)
        {
            int index = hash % SIZE;
            if (table[index].hash == hash)
            {
                wins = table[index].wins;
                visits = table[index].visits;
                ttHits++;
                return true;
            }
            return false;
        }
    };

    struct ZobristHasher
    {
        uint64_t table[121][2]; // [tile][player]
        uint64_t turn[2];       // [turn]

        ZobristHasher(); // Defined in cpp

        uint64_t getHash(const Bitboard &board, char currentTurn);
        uint64_t updateHash(uint64_t currentHash, int index, char player);
    };

    static TranspositionTable tt;
    static ZobristHasher hasher;

private:
    /**
     * @brief Selection Phase: Traverse down to a leaf node.
     *
     * Uses UCT to select the best child at each step until a node
     * with untried moves is reached.
     */
    Node *select(Node *node, Bitboard &board);

    /**
     * @brief Expansion Phase: Add a new child to the tree.
     *
     * Picks a random untried move from the leaf node and creates a new child.
     */
    Node *expand(Node *node, Bitboard &board);

    /**
     * @brief Result of a simulation.
     */
    /**
     * @brief Result of a simulation.
     */
    struct SimulationResult
    {
        char winner;
        Bitboard redMoves;
        Bitboard blueMoves;
    };

    /**
     * @brief Simulation Phase: Play a random game to completion.
     *
     * @param board The board state to simulate from.
     * @param turnColour The player whose turn it is to move next.
     * @return SimulationResult The winner and the moves played.
     */
    SimulationResult simulate(Bitboard board, char turnColour);

    /**
     * @brief Backpropagation Phase: Update stats up the tree.
     *
     * Updates visit counts and win counts for all nodes from the leaf to the root.
     * Also updates RAVE statistics.
     */
    void backpropagate(Node *node, const SimulationResult &result);

    /**
     * @brief Helper to get the opponent's colour.
     */
    char getOpponent(char colour)
    {
        return (colour == 'R') ? 'B' : 'R';
    }

private:
    /**
     * @brief Fast Xorshift Random Number Generator.
     */
    struct FastRNG
    {
        uint32_t state;

        FastRNG(uint32_t seed = 123456789) : state(seed)
        {
            if (state == 0)
                state = 123456789;
        }

        uint32_t next()
        {
            uint32_t x = state;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return state = x;
        }

        // Returns random number in [0, max-1]
        uint32_t range(uint32_t max)
        {
            return next() % max;
        }
    };

    FastRNG rng;
};
