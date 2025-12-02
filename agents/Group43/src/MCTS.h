#ifndef MCTS_H
#define MCTS_H

#include <vector>
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
     * @brief Simulation Phase: Play a random game to completion.
     * 
     * @param board The board state to simulate from.
     * @param turnColour The player whose turn it is to move next.
     * @return char The winner ('R' or 'B').
     */
    char simulate(Bitboard board, char turnColour);

    /**
     * @brief Backpropagation Phase: Update stats up the tree.
     * 
     * Updates visit counts and win counts for all nodes from the leaf to the root.
     */
    void backpropagate(Node* node, char winner);
    
    /**
     * @brief Helper to get the opponent's colour.
     */
    char getOpponent(char colour) 
    {
        return (colour == 'R') ? 'B' : 'R';
    }
};

#endif
