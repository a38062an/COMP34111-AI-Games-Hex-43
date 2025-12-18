#pragma once
#include "Bitboard.h"
#include "MCTS.h"

#include <vector>
#include <string>

using namespace std;

struct Point
{
    int x; // Represents column
    int y; // Represents row
};

struct TimeManager {
    double totalTimeMs;
    // strategy removed
    double timeRemainingMs;
    int movesPlayed;

    TimeManager(double totalTime) 
        : totalTimeMs(totalTime), timeRemainingMs(totalTime), movesPlayed(0) {}
    
    // Returns time to spend on this move
    double engage(int movesSoFar, int movesRemainingEst);
};

class HexAgent
{
public:
    /**
     * @brief Construct a new Hex Agent.
     *
     * @param colour The agent's colour ('R' or 'B').
     * @param timeLimitMs Total time budget in milliseconds.
     */
    HexAgent(char colour, double timeLimitMs = 300000.0);

    /**
     * @brief Main loop of the agent.
     *
     * Continuously reads commands from stdin and responds via stdout
     * until the game ends or the pipe is closed.
     */
    void run();

private:
    char myColour;        ///< The agent's assigned colour
    vector<string> board; ///< String representation of the board (for debugging/printing)
    Bitboard bitboard;    ///< Efficient bitset representation for MCTS
    TimeManager timeMgr;  ///< Manages time allocation
    int moveCount;        ///< Track number of moves played


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
