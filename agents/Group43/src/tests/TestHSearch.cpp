#include <iostream>
#include <cassert>
#include "../Bitboard.h"
#include "../HSearch.h"

using namespace std;

void test_direct_red_win()
{
    Bitboard b;

    // Red connects top to bottom in column 0
    for (int y = 0; y < BOARD_SIZE; y++) {
        b.set(0, y, 'R');
    }

    assert(HSearch::hasVirtualConnection(b, 'R') == true);
    assert(HSearch::hasVirtualConnection(b, 'B') == false);

    cout << "[PASS] Red direct win detected\n";
}

void test_no_win_empty_board() {
    Bitboard b;

    assert(HSearch::hasVirtualConnection(b, 'R') == false);
    assert(HSearch::hasVirtualConnection(b, 'B') == false);

    cout << "[PASS] No win on empty board\n";
}

void test_forced_win_move()
{
    Bitboard b;

    // Almost-winning red position
    for (int y = 0; y < BOARD_SIZE - 1; y++) {
        b.set(5, y, 'R');
    }

    assert(!b.checkWinRed());

    // H-Search finds a forced win for Red
    auto forcedWin = HSearch::findForcedWin(b, 'R');
    assert(forcedWin);

    // Apply the move and verify Red wins
    Bitboard next = b;
    pair<int, int> move = forcedWin.value();
    next.set(move.first, move.second, 'R');

    assert(next.checkWinRed());

    cout << "[PASS] Forced winning move detected\n";
}

int main()
{
    cout << "Running H-Search unit tests...\n";

    test_direct_red_win();
    test_no_win_empty_board();
    test_forced_win_move();

    cout << "All tests passed!\n";
    return 0;
}
