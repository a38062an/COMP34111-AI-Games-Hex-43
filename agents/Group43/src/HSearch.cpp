#include "HSearch.h"

using namespace std;

experimental::optional<pair<int,int>> HSearch::findForcedWin(const Bitboard &board, char player)
{
    // Try every legal move for virtual connections
    for (int i = 0; i < NUM_TILES; ++i)
    {
        int col = i % BOARD_SIZE;
        int row = i / BOARD_SIZE;

        if (board.isOccupied(col, row)) continue;

        Bitboard next = board;
        next.set(col, row, player);

        if (hasVirtualConnection(next, player))
        {
            return make_pair(col, row);
        }
    }
    return experimental::nullopt;
}

bool HSearch::hasVirtualConnection(const Bitboard &board, char player)
{
    if (hasDirectConnection(board, player)) return true;
    if (hasSimpleBridge(board, player)) return true;
    return false;
}

bool HSearch::opponentHasForcedWin(const Bitboard &board, char player)
{
    return false;
}

bool HSearch::hasDirectConnection(Bitboard board, char player)
{
    if (player == 'R') return board.checkWinRed();
    else return board.checkWinBlue();
}

bool HSearch::hasSimpleBridge(const Bitboard& board, char player)
{
    // Placeholder implementation
    // A real implementation would check for simple bridge patterns
    return false;
}
