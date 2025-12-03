#ifndef MCTS_H
#define MCTS_H

#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <memory>
#include <algorithm>
#include <chrono>
#include "Bitboard.h"

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

    /**
     * @brief Construct a new MCTS engine.
     * 
     * @param board The current board state.
     * @param colour The agent's colour.
     */
    MCTS(const Bitboard& board, char colour) 
        : rootBoard{board}
        , myColour{colour} 
    {}

    /**
     * @brief Run the MCTS search for a specified time duration.
     * 
     * @param timeLimitMs Time limit in milliseconds.
     * @return pair<int, int> The best move coordinates (col, row).
     */
    pair<int, int> runSearch(int timeLimitMs);

private:
    /**
     * @brief Selection Phase: Traverse down to a leaf node.
     * 
     * Uses UCT to select the best child at each step until a node
     * with untried moves is reached.
     */
    Node* select(Node* node, Bitboard& board);

    /**
     * @brief Expansion Phase: Add a new child to the tree.
     * 
     * Picks a random untried move from the leaf node and creates a new child.
     */
    Node* expand(Node* node, Bitboard& board);

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
    void backpropagate(Node* node, const SimulationResult& result);
    
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
            if (state == 0) state = 123456789;
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

#endif
