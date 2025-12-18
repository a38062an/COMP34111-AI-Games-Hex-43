/**
 * @file CppAgent.cpp
 * @brief Entry point for the C++ Hex Agent.
 *
 * This file contains the main function which parses command line arguments
 * and instantiates the HexAgent.
 */

#include <iostream>
#include <string>
#include "HexAgent.h"
#include "Bitboard.h"

using namespace std;

/**
 * @brief Main entry point.
 *
 * Usage: ./CppAgent <color> <board_size> [time_limit_ms]
 * Example: ./CppAgent R 11 300000
 *
 * @param argc Number of arguments.
 * @param argv Argument values.
 * @return int Exit code (0 for success, 1 for error).
 */
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <color> <board_size> [time_limit_ms]" << endl;
        return 1;
    }

    char colour = argv[1][0];
    
    // Default: 300 seconds (5 minutes)
    double timeLimitMs = 300000.0;

    if (argc >= 4) {
        timeLimitMs = stod(argv[3]);
    }

    Bitboard::initTables();

    // Discard board size. Assume 11x11
    HexAgent agent(colour, timeLimitMs);
    agent.run();

    return 0;
}
