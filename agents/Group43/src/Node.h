#ifndef NODE_H
#define NODE_H

#include <memory>
#include <vector>
#include <cstdint>

#include <cmath>
#include <limits>
#include <algorithm>
#include "Bitboard.h"

using namespace std;

/**
 * @brief Represents a node in the Monte Carlo Search Tree.
 */
struct Node 
{
    int moveColumn;     ///< The move column that led to this state (-1 for root)
    int moveRow;        ///< The move row that led to this state (-1 for root)
    char colour;        ///< The player who made the move at (moveColumn, moveRow)
    int visits;         ///< Number of times this node has been visited
    double raveWins;    ///< Number of RAVE wins (AMAF)
    int raveVisits;     ///< Number of RAVE visits (AMAF)
    double wins;        ///< Number of wins for the player at this node
    vector<unique_ptr<Node>> children; ///< Child nodes (managed by pool)
    Node* parent;       ///< Pointer to parent node (nullptr for root)
    vector<int16_t> untriedMoves; ///< List of legal moves not yet expanded (indices 0-120)

    /**
     * @brief Construct a new Node.
     * 
     * Automatically calculates all legal moves from the given board state
     * and populates `untriedMoves`.
     * 
     * @param column Column of the move
     * @param row Row of the move
     * @param moveColour Player who made the move
     * @param parentNode Pointer to parent
     * @param board Current board state
     */
    Node(int column, int row, char moveColour, Node* parentNode, const Bitboard& board)
        : moveColumn{column}
        , moveRow{row}
        , colour{moveColour}
        , visits{0}
        , raveWins{0.0}
        , raveVisits{0}
        , wins{0.0}
        , parent{parentNode}
    {
        // Populate untried moves
        // We iterate 0..120 and check if occupied
        for (int i = 0; i < NUM_TILES; ++i) 
        {
            int c = i % BOARD_SIZE;
            int r = i / BOARD_SIZE;
            if (!board.isOccupied(c, r)) 
            {
                untriedMoves.push_back(static_cast<int16_t>(i));
            }
        }
    }

    /**
     * @brief Check if this node is fully expanded.
     * 
     * A node is fully expanded if there are no more untried moves.
     * @return true if fully expanded.
     */
    bool isFullyExpanded() const 
    {
        return untriedMoves.empty();
    }

    /**
     * @brief Select the best child using the RAVE formula (UCT + AMAF).
     * 
     * Score = (1 - beta) * UCT + beta * RAVE
     * beta = sqrt(k / (3 * visits + k))
     * 
     * @param explorationConstant The 'c' parameter in UCT (default 1.414).
     * @param raveConstant The 'k' parameter in RAVE (default 1000).
     * @return Node* Pointer to the best child.
     */
    Node* bestChild(double explorationConstant = 1.414, double raveConstant = 1000.0) 
    {
        Node* best = nullptr;
        double bestValue = -numeric_limits<double>::infinity();

        for (const auto& childPtr : children) 
        {
            Node* child = childPtr.get();
            // UCT Part
            double uctValue = 0.0;
            if (child->visits > 0) 
            {
                uctValue = (child->wins / child->visits) + 
                           explorationConstant * sqrt(log(visits) / child->visits);
            } 
            else 
            {
                uctValue = 1e6 + (rand() % 100); 
            }

            // RAVE Part
            double raveValue = 0.0;
            if (child->raveVisits > 0) 
            {
                raveValue = child->raveWins / child->raveVisits;
            }

            // Beta Calculation
            double beta = sqrt(raveConstant / (3 * visits + raveConstant));
            
            // Combined Score
            double score;
            if (child->visits == 0) 
            {
                score = 1e6 + (rand() % 100); // Prioritize unvisited children
                if (child->raveVisits > 0) 
                {
                    score += raveValue;
                }
            } 
            else 
            {
                score = (1.0 - beta) * uctValue + beta * raveValue;
            }
            
            if (score > bestValue) 
            {
                bestValue = score;
                best = child;
            }
        }
        return best;
    }
};

#endif
