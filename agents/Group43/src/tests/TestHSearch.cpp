#include <iostream>
#include <cassert>
#include "../Bitboard.h"
#include "../HSearch.h"

using namespace std;

constexpr int CX = 6;
constexpr int CY = 5;

static void clearBoard(Bitboard& b) {
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            b.set(x, y, '0');
        }
    }
}

void test_direct_red_win()
{
    Bitboard b;

    // Red connects top to bottom in column 0
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        b.set(0, y, 'R');
    }

    assert(HSearch::hasVirtualConnection(b, 'R') == true);
    assert(HSearch::hasVirtualConnection(b, 'B') == false);

    cout << "[PASS] Red direct win detected" << endl;
}

void test_no_win_empty_board() {
    Bitboard b;

    assert(HSearch::hasVirtualConnection(b, 'R') == false);
    assert(HSearch::hasVirtualConnection(b, 'B') == false);

    cout << "[PASS] No win on empty board" << endl;
}

void test_forced_win_move()
{
    Bitboard b;

    // Almost-winning red position
    for (int y = 0; y < BOARD_SIZE - 1; y++)
    {
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

    cout << "[PASS] Forced winning move detected" << endl;
}

void test_all_simple_bridges()
{
    Bitboard b;
    clearBoard(b);

    const int offsets[6][2] = {
        {-2, 1}, {-1, -1}, {-1, 2},
        { 1,-2}, { 1, 1}, { 2,-1}
    };

    for (auto& o : offsets)
    {
        clearBoard(b);

        b.set(CX, CY, 'R');
        b.set(CX + o[0], CY + o[1], 'R');

        assert(HSearch::hasSimpleBridge(b, 'R'));
    }

    cout << "[PASS] All base-case bridge offsets detected" << endl;
}

void test_adjacent_not_bridge()
{
    Bitboard b;
    clearBoard(b);

    b.set(CX, CY, 'R');
    b.set(CX + 1, CY, 'R'); // direct neighbour

    assert(!HSearch::hasSimpleBridge(b, 'R'));

    cout << "[PASS] Adjacent stones correctly excluded" << endl;
}

void test_fat_diamond_not_bridge()
{
    Bitboard b;
    clearBoard(b);

    b.set(CX, CY, 'R');
    b.set(CX + 2, CY, 'R');  // visually close, but 0 common neighbours

    assert(!HSearch::hasSimpleBridge(b, 'R'));

    cout << "[PASS] Non-bridge diamond shape rejected" << endl;
}

void test_bridge_symmetry()
{
    Bitboard b;
    clearBoard(b);

    b.set(CX + 1, CY + 1, 'R');
    b.set(CX, CY, 'R');

    assert(HSearch::hasSimpleBridge(b, 'R'));

    cout << "[PASS] Bridge detection symmetric" << endl;
}

void test_and_recursion() {
    Bitboard b;
    clearBoard(b);

    b.set(6,5,'R');   // A
    b.set(7,6,'R');   // C
    b.set(8,7,'R');   // B

    assert(HSearch::hasVirtualConnection(b,'R'));
    cout << "[PASS] Virtual Connection AND recursion" << endl;
}

int main()
{
    cout << "Running H-Search forced win unit tests..." << endl;

    test_direct_red_win();
    test_no_win_empty_board();
    test_forced_win_move();

    cout << "All forced win tests passed!" << endl;

    cout << "Running H-Search simple bridge tests..." << endl;

    test_all_simple_bridges();
    test_adjacent_not_bridge();
    test_fat_diamond_not_bridge();
    test_bridge_symmetry();
    test_and_recursion();

    cout << "All simple bridge tests passed!" << endl;
    return 0;
}
