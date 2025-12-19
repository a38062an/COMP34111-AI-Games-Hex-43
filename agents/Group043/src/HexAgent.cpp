#include "HexAgent.h"
#include <sstream>
#include <iostream>
#include <algorithm> // for max, min
#include <string>
#include <chrono>
#include <vector>
#include <cstdlib> // rand, srand

using namespace std;

// TimeManager Implementation
double TimeManager::engage(int movesSoFar) {
    // DYNAMIC STRATEGY ONLY
    double reservedBuffer = 1500.0; // 1.5 seconds safety buffer
    double available = timeRemainingMs - reservedBuffer;
    if (available < 0) available = 100.0; // Panic mode

    // Estimate remaining moves. 
    // Hex 11x11 often ends around 40-50 moves per player.
    // Analysis of aggressive_returns.csv shows Avg TotalTurns ~26.
    // We set estimate to 30 to be safe but more aggressive.
    int expectedTotalMoves = 30;
    int movesLeft = max(1, expectedTotalMoves - movesSoFar);

    double timeForThisMove = 0.0;

    // Phases
    if (movesSoFar < 4) {
        // Opening: Play relatively fast
        timeForThisMove = 1000.0; 
    } else {
        // Mid/Late game: Distribute remaining time
        // Use a "remaining moves" divider but weighted to be safer
        // AGGRESSION FACTOR: Increased from 1.5 to 2.3 to utilize more time
        double aggression = 2.3;
        timeForThisMove = (available / movesLeft) * aggression;
        
        // Cap individual move time to avoid spending everything on one move if we have lots left
        double cap = available * 0.5; // Raised cap to 50% 
        if (timeForThisMove > cap) timeForThisMove = cap;
    }

    // Safety clamps
    if (timeForThisMove < 100.0) timeForThisMove = 100.0;
    if (timeForThisMove > available) timeForThisMove = available;

    return timeForThisMove;
}

HexAgent::HexAgent(char colour, double timeLimitMs) 
    : myColour{colour}, timeMgr(timeLimitMs), moveCount(0)
{
    srand(time(0));


}

void HexAgent::run() 
{
    string line;
    while (getline(cin, line)) 
    {
        if (line.empty()) continue;
        
        // Protocol: COMMAND;MOVE;BOARD;TURN;
        // Example: CHANGE;5,5;000,000,000;2;
        
        stringstream stringStream(line);
        string segment;
        vector<string> parts;
        
        while (getline(stringStream, segment, ';')) 
        {
            parts.push_back(segment);
        }

        // If transmission error we keep waiting for full message
        if (parts.size() < 4) continue;

        string command = parts[0];
        string moveString = parts[1];
        string boardString = parts[2];
        string turnString = parts[3];

        // Parse board
        parseBoard(boardString);
        // printBoard(); // Disabled for clean experiment output? Or keep it? keeping for debug.

        if (command == "SWAP") 
        {
            myColour = (myColour == 'R') ? 'B' : 'R';
            //cerr << "Agent swapped colour to: " << myColour << endl;
        }

        // Decide move
        Point point = makeMove();
        
        // Output move
        // Board uses x=row, y=col.
        // We store board[row][col].
        // point.x is col, point.y is row.
        // So we must output row,col -> point.y,point.x
        if (point.x != -1) {
            cout << point.y << "," << point.x << endl;
        }

        moveCount++;
    }
}

void HexAgent::printBoard() 
{
    //cerr << "Agent Board State:" << endl;
    //for (const string& row : board) 
    //{
    //    cerr << row << endl;
    //}
    //cerr << "-------------------" << endl;
}

void HexAgent::parseBoard(const string& boardString) 
{
    board.clear();
    stringstream stringStream(boardString);
    string rowString;
    int row = 0;
    while (getline(stringStream, rowString, ',')) 
    {
        board.push_back(rowString);
        for (int column = 0; column < BOARD_SIZE; ++column) 
        {
            bitboard.set(column, row, rowString[column]);
        }
        row++;
    }
}

Point HexAgent::makeMove() 
{
    // 1. OPENING / SWAP STRATEGY (First Move Only)
    if (moveCount == 0)
    {
        if (myColour == 'R') 
        {
            // "The Bot's Best Choice: ... c2, a3, or i2" 
            static const Point openings[] = {{2, 1}, {0, 2}, {8, 1}};
            int openingIndex = rand() % 3;
            Point openingMove = openings[openingIndex];
            //cerr << "Opening Strategy: Playing fair move (" << openingMove.x << "," << openingMove.y << ")" << endl;
            return {openingMove.x, openingMove.y}; 
        }
        else // myColour == 'B'
        {
            // Find opponent's move
            int opponentColumn = -1;
            int opponentRow = -1;
            
            // Optimization: Break out of BOTH loops once found
            for (int row = 0; row < BOARD_SIZE; ++row) 
            {
                for (int column = 0; column < BOARD_SIZE; ++column) 
                {
                    if (board[row][column] != '0') 
                    {
                        opponentColumn = column;
                        opponentRow = row;
                        goto foundOpponent; // Cleanest way to break nested loop in C++
                    }
                }
            }
            
            foundOpponent:

            if (opponentColumn != -1) 
            { 
                // We define the "Strong Core" as Indices 2-8 (Rows/Cols 3-9).
                bool inStrongBox = (opponentColumn >= 2 && opponentColumn <= 8) && 
                                   (opponentRow >= 2 && opponentRow <= 8);

                bool isObtuse = (opponentColumn == 0 && opponentRow == 10) || (opponentColumn == 10 && opponentRow == 0);

                if (inStrongBox || isObtuse) 
                {
                    //cerr << "Swap Strategy: Opponent move (" << opponentColumn << "," << opponentRow << ") is Strong (Red Zone). SWAPPING." << endl;
                    cout << "SWAP" << endl;
                    return {-1, -1};
                } 
                else 
                {
                     //cerr << "Swap Strategy: Opponent move (" << opponentColumn << "," << opponentRow << ") is Fair/Weak. KEEPING." << endl;
                }
            }
        }
    }

    // Caclulate time allocation
    double timeToSpend = timeMgr.engage(moveCount);
    
    // Log choice
    //cerr << "Move " << moveCount << ": Allocating " << timeToSpend << "ms (" 
    //     << timeMgr.timeRemainingMs << "ms left)" << endl;

    // Use MCTS to decide move
    MCTS mcts(bitboard, myColour);
    
    // Start Timer
    auto start = std::chrono::high_resolution_clock::now();
    
    // We pass timeToSpend. MCTS needs to respect this strictly.
    MCTS::SearchResult result = mcts.runSearch((int)timeToSpend);
    
    // Stop Timer
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    double actualTimeSpent = elapsed.count();

    pair<int, int> bestMove = result.move;

    // Deduct actual time spent (can't just use timeToSpend as MCTS might take longer than allocated)
    timeMgr.timeRemainingMs -= actualTimeSpent;

    // Log the difference
    //cerr << "Actual time spent: " << actualTimeSpent << "ms (Diff: " << (actualTimeSpent - timeToSpend) << "ms)" << endl; 

    if (bestMove.first != -1) 
    {
        return {bestMove.first, bestMove.second};
    }

    // Fallback (should not be reached if MCTS works)
    for (int row = 0; row < BOARD_SIZE; ++row) 
    {
        for (int column = 0; column < BOARD_SIZE; ++column) 
        {
            if (board[row][column] == '0') 
            {
                return {column, row};
            }
        }
    }
    return {-1, -1};
}
