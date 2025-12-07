#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <bitset>
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
    int moveColumn;                  ///< The move column that led to this state (-1 for root)
    int moveRow;                     ///< The move row that led to this state (-1 for root)
    char colour;                     ///< The player who made the move at (moveColumn, moveRow)
    int visits;                      ///< Number of times this node has been visited
    double raveWins;                 ///< Number of RAVE wins (AMAF)
    int raveVisits;                  ///< Number of RAVE visits (AMAF)
    double wins;                     ///< Number of wins for the player at this node
    vector<Node *> children;         ///< Child nodes (managed by pool)
    Node *parent;                    ///< Pointer to parent node (nullptr for root)
    uint64_t hash;                   ///< Zobrist hash of the board state at this node
    bitset<NUM_TILES> expandedMoves; ///< Moves we have already tried as children
    uint8_t remainingMoves;          ///< How many valid moves are left to try?

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
     * @param nodeHash Zobrist hash of this state
     */
    Node(int column, int row, char moveColour, Node *parentNode, const Bitboard &board, uint64_t nodeHash = 0)
        : moveColumn{column}, moveRow{row}, colour{moveColour}, visits{0}, raveWins{0.0}, raveVisits{0}, wins{0.0}, parent{parentNode}, hash{nodeHash}
    {
        // Logic: Total Tiles - (Red Tiles + Blue Tiles)
        // Note: bitset.count() is very fast (hardware instruction popcnt).
        size_t occupiedCount = (board.red | board.blue).count();
        this->remainingMoves = static_cast<uint8_t>(NUM_TILES - occupiedCount);

        extern long long g_nodeCount;
        g_nodeCount++;
    }

    ~Node()
    {
#ifdef NO_POOL
        for (Node *child : children)
        {
            delete child;
        }
#endif
    }

    /**
     * @brief Check if this node is fully expanded.
     *
     * A node is fully expanded if there are no more untried moves.
     * @return true if fully expanded.
     */
    bool isFullyExpanded() const
    {
        return remainingMoves == 0;
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
    Node *bestChild(double explorationConstant = 1.414, double raveConstant = 1000.0)
    {
        Node *best = nullptr;
        double bestValue = -numeric_limits<double>::infinity();

        // We add 1e-6 to avoid log(0) issues if called on a fresh root.
        double logParentVisits = log(this->visits + 1e-6);

        for (const auto &childPtr : children)
        {
            Node *child = childPtr;

            // If unvisited, it's infinitely interesting
            if (child->visits == 0)
                return child;

            // UCT Part
            double uctValue = (child->wins / child->visits) + explorationConstant * sqrt(logParentVisits / child->visits);

            // RAVE Part
            double raveValue = 0.0;
            if (child->raveVisits > 0)
            {
                raveValue = child->raveWins / child->raveVisits;
            }

            // Beta Calculation
            double beta = sqrt(raveConstant / (3 * visits + raveConstant));

            // Combined Score
            double score = (1.0 - beta) * uctValue + beta * raveValue;

            if (score > bestValue)
            {
                bestValue = score;
                best = child;
            }
        }
        return best;
    }
};
