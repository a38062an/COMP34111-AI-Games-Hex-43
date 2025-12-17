#pragma once

#include <vector>
#include <cstdint>
#include <bitset>
#include <cmath>
#include <limits>
#include <algorithm>
#include "Bitboard.h"

using namespace std;

struct Node
{
    // OPTIMIZATION: Use 1 byte for index instead of 8 bytes for (x,y)
    uint8_t moveIndex;               ///< The move index (0-120) that led to this state (255 for root)
    
    char colour;                     ///< The player who made the move
    uint8_t remainingMoves;          ///< How many valid moves are left to try?
    
    // Statistics (Aligned for packing)
    int visits;                      
    int raveVisits;                  
    double wins;                     
    double raveWins;                 

    Node *parent;                    
    vector<Node *> children;         
    
    uint64_t hash;                   
    bitset<NUM_TILES> expandedMoves; 

    /**
     * @brief Construct a new Node using direct index.
     */
    Node(uint8_t index, char moveColour, Node *parentNode, const Bitboard &board, uint64_t nodeHash = 0)
        : moveIndex{index}, colour{moveColour}, visits{0}, raveVisits{0}, wins{0.0}, raveWins{0.0}, parent{parentNode}, hash{nodeHash}
    {
        // Logic: Total Tiles - (Red Tiles + Blue Tiles)
        // bitset.count() is hardware optimized (popcnt)
        size_t occupiedCount = (board.red | board.blue).count();
        this->remainingMoves = static_cast<uint8_t>(NUM_TILES - occupiedCount);

        extern long long g_nodeCount;
        g_nodeCount++;
    }

    ~Node()
    {
        for (Node *child : children)
        {
            delete child;
        }
    }

    bool isFullyExpanded() const
    {
        return remainingMoves == 0;
    }

    Node *bestChild(double explorationConstant = 1.414, double raveConstant = 1000.0)
    {
        Node *best = nullptr;
        double bestValue = -numeric_limits<double>::infinity();

        // Add epsilon to avoid log(0)
        double logParentVisits = log(this->visits + 1e-6);

        for (Node *child : children)
        {
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

            // Beta Calculation (RAVE weight decreases as visits increase)
            double beta = sqrt(raveConstant / (3 * visits + raveConstant));

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