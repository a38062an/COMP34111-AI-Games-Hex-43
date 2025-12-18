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
 * Usage: ./CppAgent <color>
 * Example: ./CppAgent R
 *
 * @param argc Number of arguments.
 * @param argv Argument values.
 * @return int Exit code (0 for success, 1 for error).
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <color>" << endl;
        return 1;
    }

    char colour = argv[1][0];

    Bitboard::initTables();

    HexAgent agent(colour);
    agent.run();

    return 0;
}
