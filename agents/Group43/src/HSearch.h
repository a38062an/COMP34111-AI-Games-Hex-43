#ifndef HSEARCH_H
#define HSEARCH_H

#include "Bitboard.h"
#include <utility>
#include <experimental/optional>

using namespace std;

class HSearch {
public:
    // Returns a winning move if one is provably forced
    static experimental::optional<pair<int,int>> findForcedWin(const Bitboard& board, char player);

    // Returns true if the player has a guaranteed connection
    static bool hasVirtualConnection(const Bitboard& board, char player);

    // Used for pruning: does opponent have a forced win?
    static bool opponentHasForcedWin(const Bitboard& board, char player);

private:
    // Internal helpers (not exposed)
    static bool hasDirectConnection(Bitboard board, char player);
    static bool hasSimpleBridge(const Bitboard& board, char player);
};

#endif
