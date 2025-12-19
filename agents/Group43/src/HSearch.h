#ifndef HSEARCH_H
#define HSEARCH_H

#include "Bitboard.h"
#include <utility>
#include <experimental/optional>
#include <unordered_map>
#include <vector>

using namespace std;

/**
 * @brief Tactical layer class that handles H-Search and Virtual Connections.
 * 
 * This class is responsible for:
 * 1. Finding forced winning moves.
 * 2. Detecting direct connections, simple bridges, and virtual connections.
 */

class HSearch {
public:
    // For HexAgent: Find a forced winning move
    static experimental::optional<pair<int,int>> findForcedWin(
        const Bitboard& board,
        char player
    );

    // For MCTS: Filter moves into strong and normal using H-Search
    static void HSearch::filterMoves(
        const Bitboard& board,
        char player,
        int depth,
        bitset<NUM_TILES>& strong,
        bitset<NUM_TILES>& normal
    );

private:
    // Returns true if the player has a direct connection
    static bool hasDirectConnection(Bitboard board, char player);

    // Returns true if placing at (x,y) creates a VC up to given depth
    static bool HSearch::createsVCUpToDepth(
        const Bitboard& board,
        char player,
        int x, int y,
        int depth
    );

    // Returns true if there is a VC bounded between a and b up to given depth
    static bool HSearch::vcBoundedBetween(
        const Bitboard& board,
        char player,
        int ax, int ay,
        int bx, int by,
        int depth
    );

    // Returns true if a and b are adjacent
    static bool areAdjacent(int ax, int ay, int bx, int by);

    // Returns true if there is a simple bridge between a and b
    static bool simpleBridgeBetween(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );

    // Counts common empty neighbors between a and b
    static int countCommonEmptyNeighbors(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );

    // Get neighbors of a tile
    static vector<pair<int,int>> getNeighbors(int x, int y);
};

#endif
