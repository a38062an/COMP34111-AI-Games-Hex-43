#include "MCTS.h"
#include <cstdlib>

using namespace std;

pair<int, int> MCTS::runSearch(int timeLimitMs) 
{
    auto startTime = chrono::high_resolution_clock::now();
    
    // Root node represents the LAST move made (by opponent). 
    // So its colour is opponent's colour.
    // 
    // We pass nullptr as parent.
    // Coordinates -1, -1 indicate root.
    unique_ptr<Node> root = make_unique<Node>(-1, -1, getOpponent(myColour), nullptr, rootBoard);


    while (true) 
    {
        auto currentTime = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
        if (elapsed >= timeLimitMs) break;

        Bitboard simulationBoard = rootBoard;
        
        // 1. Selection
        Node* leaf = select(root.get(), simulationBoard);

        // 2. Expansion
        if (!simulationBoard.checkWinRed() && !simulationBoard.checkWinBlue()) 
        {
            leaf = expand(leaf, simulationBoard);
        }

        // 3. Simulation
        // Determine whose turn it is for simulation
        // If leaf->colour is Red, next turn is Blue.
        char nextTurn = getOpponent(leaf->colour);
        char winner = simulate(simulationBoard, nextTurn);

        // 4. Backpropagation
        backpropagate(leaf, winner);


    }

    // Return best move (child with most visits)
    Node* bestChild = nullptr;
    int maxVisits = -1;

    for (const auto& child : root->children) 
    {
        if (child->visits > maxVisits) 
        {
            maxVisits = child->visits;
            bestChild = child.get();
        }
    }

    if (bestChild) 
    {
        return {bestChild->x, bestChild->y};
    }
    
    // Fallback if no search happened (should not happen)
    return {-1, -1};
}

Node* MCTS::select(Node* node, Bitboard& board) 
{
    while (node->isFullyExpanded() && !node->children.empty()) 
    {
        node = node->bestChild();
        board.set(node->x, node->y, node->colour);
    }
    return node;
}

Node* MCTS::expand(Node* node, Bitboard& board) 
{
    if (node->untriedMoves.empty()) return node;

    // 1. Pick a random move that we haven't explored yet from this state.
    // 'untriedMoves' holds all valid moves from this node that don't have a child node yet.
    int index = rand() % node->untriedMoves.size();
    pair<int, int> chosenMove = node->untriedMoves[index];
    
    // 2. Remove this move from 'untriedMoves' so we don't expand it again later.
    // We swap with the back and pop to do this efficiently in O(1).
    node->untriedMoves[index] = node->untriedMoves.back();
    node->untriedMoves.pop_back();

    // 3. Determine who made this move.
    // The 'node' represents the state after the *previous* player moved.
    // So the move we just picked is made by the *current* player (opponent of node->colour).
    char childColour = getOpponent(node->colour);
    
    // 4. Update the board state with this new move.
    board.set(chosenMove.first, chosenMove.second, childColour);
    
    // 5. Create a new child node representing this new board state.
    // The child node will automatically calculate its own 'untriedMoves' (legal moves) in its constructor.
    auto child = make_unique<Node>(chosenMove.first, chosenMove.second, childColour, node, board);

    // 6. Add this new child to the current node's list of children.
    // We use std::move because 'child' is a unique_ptr and ownership is being transferred to the vector.
    node->children.push_back(std::move(child));
    
    // 7. Return the raw pointer to the newly created child so we can run a simulation from it.
    return node->children.back().get();
}

char MCTS::simulate(Bitboard board, char turnColour) 
{
    // Random rollout
    while (true) 
    {
        if (board.checkWinRed()) return 'R';
        if (board.checkWinBlue()) return 'B';

        // Find all empty spots
        vector<pair<int, int>> emptySpots;
        for (int row = 0; row < BOARD_SIZE; ++row) 
        {
            for (int column = 0; column < BOARD_SIZE; ++column) 
            {
                if (!board.isOccupied(column, row)) 
                {
                    emptySpots.push_back({column, row});
                }
            }
        }

        if (emptySpots.empty()) break; // Draw edge case (should never happen)

        // Pick random
        int index = rand() % emptySpots.size();
        board.set(emptySpots[index].first, emptySpots[index].second, turnColour);
        
        turnColour = getOpponent(turnColour);
    }
    return '0';
}

void MCTS::backpropagate(Node* node, char winner) 
{
    // Walk up the tree from the leaf node to the root.
    while (node != nullptr) 
    {
        node->visits++;
        
        // If the player who just moved at this node eventually won the game,
        // we increment their win count.
        if (node->colour == winner) 
        {
            node->wins++;
        }
        
        // Move up to the parent
        node = node->parent;
    }
}
