#pragma once

#include <bitset>
#include <iostream>
#include <array>

using namespace std;

// 11x11 = 121 tiles
constexpr int BOARD_SIZE = 11;
constexpr int NUM_TILES = 121;

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
    inline static array<AdjacencyList, NUM_TILES> ADJACENCY;

public:
    bitset<NUM_TILES> red;  ///< Bitset for Red pieces (1 = occupied by Red)
    bitset<NUM_TILES> blue; ///< Bitset for Blue pieces (1 = occupied by Blue)

    // Static initializer function (call once in main or static block)
    static void initTables()
    {
        // Optimization: Check if Tile 0 has neighbors to see if we already initialized.
        if (ADJACENCY[0].count != 0)
            return;

        int neighbors[6][2] = {{0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}};

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
     * @brief Place a piece on the board (using direct index).
     *
     * @param index Flattened board index (row * BOARD_SIZE + col)
     * @param colour 'R' for Red, 'B' for Blue, or '0' to clear.
     */
    void set(int index, char colour)
    {
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
     * @brief Place a piece on the board (using coordinates).
     *
     * @param x Column index (0-10)
     * @param y Row index (0-10)
     * @param colour 'R' for Red, 'B' for Blue, or '0' to clear.
     */
    void set(int x, int y, char colour)
    {
        // Delegates to the index-based overload
        set(y * BOARD_SIZE + x, colour);
    }

    /**
     * @brief Check if a tile is occupied by either player (using direct index).
     *
     * @param index Flattened board index (row * BOARD_SIZE + col)
     * @return true if occupied, false otherwise.
     */
    bool isOccupied(int index) const
    {
        return red.test(index) || blue.test(index);
    }

    /**
     * @brief Check if a tile is occupied by either player (using coordinates).
     *
     * @param x Column index
     * @param y Row index
     * @return true if occupied, false otherwise.
     */
    bool isOccupied(int x, int y) const
    {
        // Delegates to the index-based overload for consistency
        return isOccupied(y * BOARD_SIZE + x);
    }

    /**
     * @brief Get the piece at a specific location (using direct index).
     * @param index Flattened board index
     * @return 'R', 'B', or '0'
     */
    char get(int index) const
    {
        if (red.test(index)) return 'R';
        if (blue.test(index)) return 'B';
        return '0';
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
        if (red.test(index)) return 'R';
        if (blue.test(index)) return 'B';
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
        bitset<NUM_TILES> visited;
        int stack[NUM_TILES];
        int stackSize = 0;

        for (int i = 0; i < BOARD_SIZE; ++i) 
        {
            if (red.test(i)) { visited.set(i); stack[stackSize++] = i; }
        }

        while (stackSize > 0)
        {
            int curr = stack[--stackSize];
            
            // Fast check for bottom row
            if (curr >= NUM_TILES - BOARD_SIZE) return true;

            const auto &adj = ADJACENCY[curr];
            for (int i = 0; i < adj.count; ++i)
            {
                int n = adj.neighbors[i];
                if (red.test(n) && !visited.test(n))
                {
                    visited.set(n);
                    stack[stackSize++] = n;
                }
            }
        }
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

        // Blue starts at Left Column: 0, 11, 22, 33...
        for (int i = 0; i < NUM_TILES; i += BOARD_SIZE) 
        {
            if (blue.test(i)) { visited.set(i); stack[stackSize++] = i; }
        }

        while (stackSize > 0)
        {
            int curr = stack[--stackSize];
            
            // Logic: (curr % 11 == 10) -> (curr + 1) % 11 == 0
            if ((curr + 1) % BOARD_SIZE == 0) return true;

            const auto &adj = ADJACENCY[curr];
            for (int i = 0; i < adj.count; ++i)
            {
                int n = adj.neighbors[i];
                if (blue.test(n) && !visited.test(n))
                {
                    visited.set(n);
                    stack[stackSize++] = n;
                }
            }
        }
        return false;
    }
};
