#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>

#include "Bitboard.h"
#include "MCTS.h"

using namespace std;

struct Point
{
    int x; // Represents column
    int y; // Represents row
};

/**
 * @brief Main agent class that handles game protocol and logic.
 *
 * This class is responsible for:
 * 1. Parsing input commands from the game engine.
 * 2. Maintaining the board state.
 * 3. Invoking the MCTS engine to decide moves.
 * 4. Sending moves back to the engine.
 */
class HexAgent
{
public:
    /**
     * @brief Construct a new Hex Agent.
     *
     * @param colour The agent's colour ('R' or 'B').
     * @param size The board size (usually 11).
     */
    HexAgent(char colour, int size);

    /**
     * @brief Main loop of the agent.
     *
     * Continuously reads commands from stdin and responds via stdout
     * until the game ends or the pipe is closed.
     */
    void run();

private:
    char myColour;        ///< The agent's assigned colour
    int boardSize;        ///< The size of the board (e.g., 11)
    vector<string> board; ///< String representation of the board (for debugging/printing)
    Bitboard bitboard;    ///< Efficient bitset representation for MCTS

    /**
     * @brief Print the current board state to stderr for debugging.
     */
    void printBoard();

    /**
     * @brief Parse the board string received from the engine.
     *
     * Updates both the string `board` and the `bitboard`.
     *
     * @param boardString Comma-separated string of rows (e.g., "000,0R0,00B").
     */
    void parseBoard(const string &boardString);

    /**
     * @brief Decide on the best move to make.
     *
     * Uses MCTS to search for the optimal move.
     *
     * @return Point The coordinates of the chosen move.
     */
    Point makeMove();
};
