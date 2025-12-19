#include "HSearch.h"

using namespace std;

// For HexAgent: Find a forced winning move
experimental::optional<pair<int,int>> HSearch::findForcedWin(
    const Bitboard &board,
    char player
) {
    // Try every legal move for virtual connections
    for (int i = 0; i < NUM_TILES; ++i)
    {
        int col = i % BOARD_SIZE;
        int row = i / BOARD_SIZE;

        if (board.isOccupied(col, row)) continue;

        Bitboard next = board;
        next.set(col, row, player);

        if (hasDirectConnection(next, player)) return make_pair(col, row);
    }
    return experimental::nullopt;
}

// For MCTS: Filter moves into strong and normal using H-Search
void HSearch::filterMoves(
    const Bitboard& board,
    char player,
    int depth,
    bitset<NUM_TILES>& strong,
    bitset<NUM_TILES>& normal
) {    
    strong.reset();
    normal.reset();

    for (int i = 0; i < NUM_TILES; ++i) {
        int x = i % BOARD_SIZE;
        int y = i / BOARD_SIZE;

        if (board.isOccupied(x, y))
            continue;

        Bitboard next = board;
        next.set(x, y, player);

        // Hard filter: immediate opponent win
        char opponent = (player == 'R') ? 'B' : 'R';
        if (hasDirectConnection(next, opponent)) continue;

        // Soft signal: bounded VC
        if (createsVCUpToDepth(board, player, x, y, depth)) {
            strong.set(i);
        } else {
            normal.set(i);
        }
    }
}

// Returns true if the player has a direct connection
bool HSearch::hasDirectConnection(Bitboard board, char player)
{
    if (player == 'R') return board.checkWinRed();
    else return board.checkWinBlue();
}

// Returns true if placing at (x,y) creates a VC up to given depth
bool HSearch::createsVCUpToDepth(
    const Bitboard& board,
    char player,
    int x, int y,
    int depth
) {
    Bitboard next = board;
    next.set(x, y, player);

    // Collect player stones
    vector<pair<int,int>> stones;
    for (int yy = 0; yy < BOARD_SIZE; ++yy)
        for (int xx = 0; xx < BOARD_SIZE; ++xx)
            if (next.get(xx, yy) == player)
                stones.emplace_back(xx, yy);

    // Check whether the new stone participates in any VC
    for (auto& stone : stones) {
        int sx = stone.first;
        int sy = stone.second;
        if (sx == x && sy == y) continue;

        if (vcBoundedBetween(next, player, x, y, sx, sy, depth))
            return true;
    }

    return false;
}

// Returns true if there is a VC bounded between a and b up to given depth
bool HSearch::vcBoundedBetween(
    const Bitboard& board,
    char player,
    int ax, int ay,
    int bx, int by,
    int depth
) {
    // Base cases
    if (areAdjacent(ax, ay, bx, by)) return true;

    if (simpleBridgeBetween(board, ax, ay, bx, by)) return true;

    if (depth == 0) return false;

    // AND rule (bounded)
    for (int cy = 0; cy < BOARD_SIZE; ++cy) {
        for (int cx = 0; cx < BOARD_SIZE; ++cx) {

            if (board.get(cx, cy) != player) continue;

            if ((cx == ax && cy == ay) || (cx == bx && cy == by)) continue;

            // Locality pruning
            if (abs(cx - ax) > 3 || abs(cy - ay) > 3) continue;

            if (vcBoundedBetween(board, player, ax, ay, cx, cy, depth - 1) &&
                vcBoundedBetween(board, player, cx, cy, bx, by, depth - 1))
                return true;
        }
    }

    return false;
}

// Returns true if a and b are adjacent
bool HSearch::areAdjacent(int ax, int ay, int bx, int by)
{
    static const int neigh[6][2] = {
        {0, -1}, {1, -1},
        {-1, 0}, {1,  0},
        {-1, 1}, {0,  1}
    };

    for (auto& n : neigh)
    {
        if (ax + n[0] == bx && ay + n[1] == by) return true;
    }
    return false;
}

// Returns true if there is a simple bridge between a and b
bool HSearch::simpleBridgeBetween(
    const Bitboard& board,
    int ax, int ay,
    int bx, int by
) {
    // must not already be adjacent
    if (areAdjacent(ax, ay, bx, by))
        return false;

    // must share at least 2 empty neighbours
    return countCommonEmptyNeighbors(board, ax, ay, bx, by) >= 2;
}

// Counts common empty neighbors between a and b
int HSearch::countCommonEmptyNeighbors(
    const Bitboard& board,
    int ax, int ay,
    int bx, int by
) {
    auto neighA = getNeighbors(ax, ay);
    auto neighB = getNeighbors(bx, by);

    int count = 0;

    for (auto& na : neighA) {
        for (auto& nb : neighB) {
            if (na == nb && !board.isOccupied(na.first, na.second)) count++;
        }
    }
    return count;
}

// Get neighbors of a tile
vector<pair<int,int>> HSearch::getNeighbors(int x, int y)
{
    static const int neigh[6][2] = {
        {0, -1}, {1, -1},
        {-1, 0}, {1, 0},
        {-1, 1}, {0, 1}
    };

    vector<pair<int,int>> result;

    for (auto& n : neigh)
    {
        int nx = x + n[0];
        int ny = y + n[1];

        if (nx >= 0 && nx < BOARD_SIZE &&
            ny >= 0 && ny < BOARD_SIZE)
            result.emplace_back(nx, ny);
    }
    return result;
}
