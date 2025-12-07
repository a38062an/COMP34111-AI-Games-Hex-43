#ifndef BITBOARD_H
#define BITBOARD_H

#include <vector>
#include <bitset>
#include <iostream>

using namespace std;

// 11x11 = 121 tiles
const int BOARD_SIZE = 11;
const int NUM_TILES = 121;

// A struct to hold pre-calculated neighbors
struct AdjacencyList
{
    int count;        // How many neighbors (usually 6, fewer at edges)
    int neighbors[6]; // The direct indices (0-120)
};

/**
 * @brief Efficient board representation using std::bitset.
 *
 * Uses two bitsets to represent the board state for Red and Blue players.
 * This allows for O(1) set/get operations and very fast win checking.
 */
class Bitboard
{
private:
    // Static lookup table, shared by ALL Bitboards
    inline static vector<AdjacencyList> ADJACENCY;

public:
    bitset<NUM_TILES> red;  ///< Bitset for Red pieces (1 = occupied by Red)
    bitset<NUM_TILES> blue; ///< Bitset for Blue pieces (1 = occupied by Blue)

    // Static initializer function (call once in main or static block)
    static void initTables()
    {
        if (!ADJACENCY.empty())
            return;

        int neighbors[6][2] = {{0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}};

        ADJACENCY.resize(NUM_TILES);
        for (int i = 0; i < NUM_TILES; ++i)
        {
            int cx = i % BOARD_SIZE;
            int cy = i / BOARD_SIZE;

            ADJACENCY[i].count = 0;
            for (auto &offset : neighbors)
            {
                int nx = cx + offset[0];
                int ny = cy + offset[1];
                if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE)
                {
                    // Store the final index directly
                    ADJACENCY[i].neighbors[ADJACENCY[i].count++] = ny * BOARD_SIZE + nx;
                }
            }
        }
    }

    /**
     * @brief Construct a new empty Bitboard.
     */
    Bitboard()
    {
        red.reset();
        blue.reset();
    }

    /**
     * @brief Place a piece on the board.
     *
     * @param x Column index (0-10)
     * @param y Row index (0-10)
     * @param colour 'R' for Red, 'B' for Blue, or '0' to clear.
     */
    void set(int x, int y, char colour)
    {
        int index = y * BOARD_SIZE + x; // x is col, y is row
        if (colour == 'R')
        {
            red.set(index);
            blue.reset(index);
        }
        else if (colour == 'B')
        {
            blue.set(index);
            red.reset(index);
        }
        else
        {
            red.reset(index);
            blue.reset(index);
        }
    }

    /**
     * @brief Check if a tile is occupied by either player.
     *
     * @param x Column index
     * @param y Row index
     * @return true if occupied, false otherwise.
     */
    bool isOccupied(int x, int y) const
    {
        int index = y * BOARD_SIZE + x;
        return red.test(index) || blue.test(index);
    }

    /**
     * @brief Get the piece at a specific location.
     *
     * @param x Column index
     * @param y Row index
     * @return 'R', 'B', or '0' (empty).
     */
    char get(int x, int y) const
    {
        int index = y * BOARD_SIZE + x;
        if (red.test(index))
            return 'R';
        if (blue.test(index))
            return 'B';
        return '0';
    }

    /**
     * @brief Check if Red has won (connected Top to Bottom).
     *
     * Uses a Depth-First Search (DFS) starting from the top row.
     * @return true if Red has a winning path.
     */
    bool checkWinRed()
    {
        // Simple DFS/Floodfill on the bitset
        bitset<NUM_TILES> visited;

        // Use fixed-size array on the stack
        int stack[NUM_TILES];
        int stackSize = 0;

        // Add all Red stones in the top row (y=0) to stack
        for (int column = 0; column < BOARD_SIZE; ++column)
        {
            // row 0 means index == column
            if (red.test(column))
            {
                visited.set(column);
                stack[stackSize++] = column;
            }
        }

        while (stackSize > 0)
        {
            int currentIndex = stack[--stackSize]; // Pop from stack

            // If we reached the bottom row (y=10), Red wins
            if (currentIndex / BOARD_SIZE == BOARD_SIZE - 1)
                return true;

            // ULTRA-FAST NEIGHBOR LOOP
            // No math, no boundary checks, just memory lookups
            const auto &adj = ADJACENCY[currentIndex];

            for (int i = 0; i < adj.count; ++i)
            {
                int neighborIndex = adj.neighbors[i];

                if (red.test(neighborIndex) && !visited.test(neighborIndex))
                {
                    visited.set(neighborIndex);
                    stack[stackSize++] = neighborIndex;
                }
            }
        }
        // After exploring all neighbors, if we haven't reached the bottom row, Red has not won
        return false;
    }

    /**
     * @brief Check if Blue has won (connected Left to Right).
     *
     * Uses a Depth-First Search (DFS) starting from the left column.
     * @return true if Blue has a winning path.
     */
    bool checkWinBlue()
    {
        bitset<NUM_TILES> visited;

        int stack[NUM_TILES];
        int stackSize = 0;

        // Add all Blue stones in the left column (x=0)
        for (int row = 0; row < BOARD_SIZE; ++row)
        {
            int index = row * BOARD_SIZE; // x=0
            if (blue.test(index))
            {
                visited.set(index);
                stack[stackSize++] = index;
            }
        }

        while (stackSize > 0)
        {
            int currentIndex = stack[--stackSize];

            // If we reached the right column (x=10), Blue wins
            if (currentIndex % BOARD_SIZE == BOARD_SIZE - 1)
                return true;

            const auto &adj = ADJACENCY[currentIndex];

            for (int i = 0; i < adj.count; i++)
            {
                int neighbourIndex = adj.neighbors[i];

                if (blue.test(neighbourIndex) && !visited.test(neighbourIndex))
                {
                    visited.set(neighbourIndex);
                    stack[stackSize++] = neighbourIndex;
                }
            }
        }
        return false;
    }
};

#endif
