#include "HSearch.h"

using namespace std;

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

bool HSearch::hasDirectConnection(Bitboard board, char player)
{
    if (player == 'R') return board.checkWinRed();
    else return board.checkWinBlue();
}

bool HSearch::hasSimpleBridge(const Bitboard& board, char player)
{
    for (int ay = 0; ay < BOARD_SIZE; ++ay)
    {
        for (int ax = 0; ax < BOARD_SIZE; ++ax)
        {
            if (board.get(ax, ay) != player)
                continue;

            for (int by = 0; by < BOARD_SIZE; ++by) {
                for (int bx = 0; bx < BOARD_SIZE; ++bx)
                {
                    if (bx == ax && by == ay) continue;
                    if (board.get(bx, by) != player) continue;
                    if (simpleBridgeBetween(board, ax, ay, bx, by)) return true;
                }
            }
        }
    }
    return false;
}

bool HSearch::hasVirtualConnection(const Bitboard& board, char player) {
    unordered_map<uint64_t,bool> memo;

    // collect all stones
    vector<pair<int,int>> stones;
    for (int y = 0; y < BOARD_SIZE; ++y)
        for (int x = 0; x < BOARD_SIZE; ++x)
            if (board.get(x,y) == player)
                stones.emplace_back(x,y);

    // check all pairs
    for (size_t i = 0; i < stones.size(); ++i)
    {
        for (size_t j = i+1; j < stones.size(); ++j)
        {
            int ax = stones[i].first;
            int ay = stones[i].second;
            int bx = stones[j].first;
            int by = stones[j].second;

            if (vcRecursive(board, player, ax,ay, bx,by, memo)) return true;
        }
    }

    return false;
}

// TODO: Implement opponentHasForcedWin
bool HSearch::opponentHasForcedWin(const Bitboard &board, char player)
{
    return false;
}

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

bool HSearch::areAdjacent(int x1, int y1, int x2, int y2)
{
    static const int neigh[6][2] = {
        {0, -1}, {1, -1},
        {-1, 0}, {1, 0},
        {-1, 1}, {0, 1}
    };

    for (auto& n : neigh)
    {
        if (x1 + n[0] == x2 && y1 + n[1] == y2) return true;
    }
    return false;
}

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

bool HSearch::vcRecursive(
    const Bitboard& board,
    char player,
    int ax, int ay,
    int bx, int by,
    unordered_map<uint64_t,bool>& memo
) {
    uint64_t key = pack(ax,ay,bx,by);
    auto it = memo.find(key);
    if (it != memo.end()) return it->second;

    // ---- Base cases ----

    // adjacency...
    if (areAdjacent(ax,ay,bx,by)) {
        memo[key] = true;
        return true;
    }

    // ...OR simple bridge
    if (simpleBridgeBetween(board, ax,ay, bx,by)) {
        memo[key] = true;
        return true;
    }

    // AND recursion

    // Try intermediate stones C
    for (int cy = 0; cy < BOARD_SIZE; ++cy) {
        for (int cx = 0; cx < BOARD_SIZE; ++cx) {

            if (cx == ax && cy == ay) continue;
            if (cx == bx && cy == by) continue;
            if (board.get(cx,cy) != player) continue;

            // Pruning: local only
            if (abs(cx - ax) > 3 || abs(cy - ay) > 3) continue;

            if (vcRecursive(board, player, ax,ay, cx,cy, memo) &&
                vcRecursive(board, player, cx,cy, bx,by, memo))
            {
                memo[key] = true;
                return true;
            }
        }
    }

    memo[key] = false;
    return false;
}

uint64_t HSearch::pack(int ax,int ay,int bx,int by) {
    // order-independent
    if (ax > bx || (ax == bx && ay > by)) {
        swap(ax,bx);
        swap(ay,by);
    }
    return  ((uint64_t)ax << 48)
          | ((uint64_t)ay << 32)
          | ((uint64_t)bx << 16)
          |  (uint64_t)by;
}
