#ifndef HSEARCH_H
#define HSEARCH_H

#include "Bitboard.h"
#include <utility>
#include <experimental/optional>
#include <unordered_map>

using namespace std;

class HSearch {
public:
    // Returns a winning move if one is provably forced
    static experimental::optional<pair<int,int>> findForcedWin(const Bitboard& board, char player);

    // Returns true if the player has a guaranteed connection
    static bool hasVirtualConnection(const Bitboard& board, char player);

    // Used for pruning: does opponent have a forced win?
    static bool opponentHasForcedWin(const Bitboard& board, char player);

    static bool hasDirectConnection(Bitboard board, char player);

    static bool hasSimpleBridge(const Bitboard& board, char player);

    static bool vcRecursive(
        const Bitboard& board,
        char player,
        int ax, int ay,
        int bx, int by,
        unordered_map<uint64_t,bool>& memo
    );

private:
    static bool areAdjacent(int x1, int y1, int x2, int y2);
    static vector<pair<int,int>> getNeighbors(int x, int y);
    static int countCommonEmptyNeighbors(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );
    static bool simpleBridgeBetween(
        const Bitboard& board,
        int ax, int ay,
        int bx, int by
    );
    static uint64_t pack(int ax,int ay,int bx,int by);
};

#endif
