#ifndef NODE_H
#define NODE_H

#include <vector>
#include <memory>
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
    int x, y;           ///< The move coordinates that led to this state (-1, -1 for root)
    char colour;        ///< The player who made the move at (x, y)
    int visits;         ///< Number of times this node has been visited
    double wins;        ///< Number of wins for the player at this node
    vector<unique_ptr<Node>> children; ///< Child nodes
    Node* parent;       ///< Pointer to parent node (nullptr for root)
    vector<pair<int, int>> untriedMoves; ///< List of legal moves not yet expanded

    /**
     * @brief Construct a new Node.
     * 
     * Automatically calculates all legal moves from the given board state
     * and populates `untriedMoves`.
     * 
     * @param moveX Column of the move
     * @param moveY Row of the move
     * @param moveColour Player who made the move
     * @param parentNode Pointer to parent
     * @param board Current board state
     */
    Node(int moveX, int moveY, char moveColour, Node* parentNode, const Bitboard& board)
        : x{moveX}
        , y{moveY}
        , colour{moveColour}
        , visits{0}
        , wins{0.0}
        , parent{parentNode}
    {
        // Populate untried moves
        for (int row = 0; row < BOARD_SIZE; ++row) 
        {
            for (int column = 0; column < BOARD_SIZE; ++column) 
            {
                if (!board.isOccupied(column, row)) 
                {
                    untriedMoves.push_back({column, row});
                }
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
     * @brief Select the best child using the UCT formula.
     * 
     * UCT = (wins / visits) + c * sqrt(log(parent_visits) / visits)
     * 
     * @param explorationConstant The 'c' parameter in UCT (default 1.414).
     * @return Node* Pointer to the best child.
     */
    Node* bestChild(double explorationConstant = 1.414) 
    {
        Node* best = nullptr;
        double bestValue = -numeric_limits<double>::infinity();

        for (const auto& child : children) 
        {
            double uctValue = (child->wins / child->visits) + 
                              explorationConstant * sqrt(log(visits) / child->visits);
            
            if (uctValue > bestValue) 
            {
                bestValue = uctValue;
                best = child.get();
            }
        }
        return best;
    }
};

#endif
