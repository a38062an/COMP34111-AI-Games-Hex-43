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
 * 1. Finding forced winning moves using H-Search.
 * 2. Detecting direct connections, simple bridges, and virtual connections.
 * 3. Detecting opponent's forced wins (TODO).
 */

class HSearch {
public:
    // Returns a winning move if one is provably forced
    static experimental::optional<pair<int,int>> findForcedWin(
        const Bitboard& board,
        char player
    );

    // Returns true if the player has a direct connection
    static bool hasDirectConnection(Bitboard board, char player);

    // Returns true if the player has a simple bridge
    static bool hasSimpleBridge(const Bitboard& board, char player);

    // Returns true if the player has a guaranteed connection
    static bool hasVirtualConnection(const Bitboard& board, char player);

    // TODO:
    // Used for pruning: returns true if opponent has a forced win
    static bool opponentHasForcedWin(const Bitboard& board, char player);

private:
    static bool simpleBridgeBetween(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );

    static bool areAdjacent(int x1, int y1, int x2, int y2);

    static int countCommonEmptyNeighbors(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );

    static vector<pair<int,int>> getNeighbors(int x, int y);

    static bool vcRecursive(
        const Bitboard& board,
        char player,
        int ax, int ay,
        int bx, int by,
        unordered_map<uint64_t,bool>& memo
    );

    static uint64_t pack(int ax,int ay,int bx,int by);
};

#endif
