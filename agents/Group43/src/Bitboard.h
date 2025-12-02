#ifndef BITBOARD_H
#define BITBOARD_H

#include <vector>
#include <bitset>
#include <iostream>

using namespace std;

// 11x11 = 121 tiles
const int BOARD_SIZE = 11;
const int NUM_TILES = 121;

/**
 * @brief Efficient board representation using std::bitset.
 * 
 * Uses two bitsets to represent the board state for Red and Blue players.
 * This allows for O(1) set/get operations and very fast win checking.
 */
class Bitboard 
{
public:
    bitset<NUM_TILES> red;  ///< Bitset for Red pieces (1 = occupied by Red)
    bitset<NUM_TILES> blue; ///< Bitset for Blue pieces (1 = occupied by Blue)

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
        // Simple DFS/Floodfill on the bitset
        bitset<NUM_TILES> visited;
        vector<int> stack;

        // Add all Red stones in the top row (y=0) to stack
        for (int column = 0; column < BOARD_SIZE; ++column) 
        {
            int index = column; // y=0, so index = column
            if (red.test(index)) 
            {
                stack.push_back(index);
                visited.set(index);
            }
        }

        while (!stack.empty()) 
        {
            int currentIndex = stack.back();
            stack.pop_back();

            int currentX = currentIndex % BOARD_SIZE;
            int currentY = currentIndex / BOARD_SIZE;

            // If we reached the bottom row (y=10), Red wins
            if (currentY == BOARD_SIZE - 1) return true;
            
            // Explore all neighbors
            int neighbors[6][2] = {
                {0, -1}, {1, -1},
                {-1, 0}, {1, 0},
                {-1, 1}, {0, 1}
            };

            for (auto& offset : neighbors) 
            {
                int neighborX = currentX + offset[0];
                int neighborY = currentY + offset[1];

                if (neighborX >= 0 && neighborX < BOARD_SIZE && neighborY >= 0 && neighborY < BOARD_SIZE) 
                {
                    int neighborIndex = neighborY * BOARD_SIZE + neighborX;
                    if (red.test(neighborIndex) && !visited.test(neighborIndex)) 
                    {
                        visited.set(neighborIndex);
                        stack.push_back(neighborIndex);
                    }
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
        vector<int> stack;

        // Add all Blue stones in the left column (x=0)
        for (int row = 0; row < BOARD_SIZE; ++row) 
        {
            int index = row * BOARD_SIZE; // x=0
            if (blue.test(index)) 
            {
                stack.push_back(index);
                visited.set(index);
            }
        }

        while (!stack.empty()) 
        {
            int currentIndex = stack.back();
            stack.pop_back();

            int currentX = currentIndex % BOARD_SIZE;
            int currentY = currentIndex / BOARD_SIZE;

            // If we reached the right column (x=10), Blue wins
            if (currentX == BOARD_SIZE - 1) return true;

            int neighbors[6][2] = {
                {0, -1}, {1, -1},
                {-1, 0}, {1, 0},
                {-1, 1}, {0, 1}
            };

            for (auto& offset : neighbors) 
            {
                int neighborX = currentX + offset[0];
                int neighborY = currentY + offset[1];

                if (neighborX >= 0 && neighborX < BOARD_SIZE && neighborY >= 0 && neighborY < BOARD_SIZE) 
                {
                    int neighborIndex = neighborY * BOARD_SIZE + neighborX;
                    if (blue.test(neighborIndex) && !visited.test(neighborIndex)) 
                    {
                        visited.set(neighborIndex);
                        stack.push_back(neighborIndex);
                    }
                }
            }
        }
        return false;
    }
};

#endif
