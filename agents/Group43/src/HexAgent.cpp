#include "HexAgent.h"
#include "HSearch.h"

using namespace std;

HexAgent::HexAgent(char colour, int size) 
    : myColour{colour}
    , boardSize{size} 
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
        printBoard();

        if (command == "SWAP") 
        {
            myColour = (myColour == 'R') ? 'B' : 'R';
            cerr << "Agent swapped colour to: " << myColour << endl;
        }

        // Decide move
        Point point = makeMove();
        
        // Output move
        // Board uses x=row, y=col.
        // We store board[row][col].
        // point.x is col, point.y is row.
        // So we must output row,col -> point.y,point.x
        cout << point.y << "," << point.x << endl;
    }
}

void HexAgent::printBoard() 
{
    cerr << "Agent Board State:" << endl;
    for (const string& row : board) 
    {
        cerr << row << endl;
    }
    cerr << "-------------------" << endl;
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
        for (int column = 0; column < boardSize; ++column) 
        {
            bitboard.set(column, row, rowString[column]);
        }
        row++;
    }
}

Point HexAgent::makeMove() 
{
    // Tactical Solver: Check for forced wins
    auto forcedWin = HSearch::findForcedWin(bitboard, myColour);
    if (forcedWin)
    {
        pair<int, int> move = forcedWin.value();
        return {move.first, move.second};
    }

    // Use MCTS to decide move
    // Time limit: 4 second (4000ms) for now
    // TODO: Dynamic time management based on remaining time
    
    MCTS mcts(bitboard, myColour);
    pair<int, int> bestMove = mcts.runSearch(4000);

    if (bestMove.first != -1) 
    {
        return {bestMove.first, bestMove.second};
    }

    // Fallback (should not be reached if MCTS works)
    for (int row = 0; row < boardSize; ++row) 
    {
        for (int column = 0; column < boardSize; ++column) 
        {
            if (board[row][column] == '0') 
            {
                return {column, row};
            }
        }
    }
    return {-1, -1};
}
