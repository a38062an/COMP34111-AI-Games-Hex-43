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
    auto root = make_unique<Node>(-1, -1, getOpponent(myColour), nullptr, rootBoard);

    while (true) 
    {
        auto currentTime = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
        if (elapsed >= timeLimitMs) 
        {
            break;
        }

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
        SimulationResult result = simulate(simulationBoard, nextTurn);

        // 4. Backpropagation
        backpropagate(leaf, result);
    }

    // Return best move (child with most visits)
    Node* bestChild = nullptr;
    int maxVisits = -1;

    for (const auto& childPtr : root->children) 
    {
        Node* child = childPtr.get();
        if (child->visits > maxVisits) 
        {
            maxVisits = child->visits;
            bestChild = child;
        }
    }

    if (bestChild) 
    {
        return {bestChild->moveColumn, bestChild->moveRow};
    }
    
    // Fallback if no search happened (should not happen)
    return {-1, -1};
}

Node* MCTS::select(Node* node, Bitboard& board) 
{
    while (node->isFullyExpanded() && !node->children.empty()) 
    {
        node = node->bestChild();
        board.set(node->moveColumn, node->moveRow, node->colour);
    }
    return node;
}

Node* MCTS::expand(Node* node, Bitboard& board) 
{
    if (node->untriedMoves.empty()) 
    {
        return node;
    }

    // 1. Pick a random move that we haven't explored yet from this state.
    // 'untriedMoves' holds all valid moves from this node that don't have a child node yet.
    // Use FastRNG
    int index = rng.range(node->untriedMoves.size());
    int16_t moveIndex = node->untriedMoves[index];
    
    // 2. Remove this move from 'untriedMoves' so we don't expand it again later.
    // We swap with the back and pop to do this efficiently in O(1).
    node->untriedMoves[index] = node->untriedMoves.back();
    node->untriedMoves.pop_back();

    // 3. Determine who made this move.
    // The 'node' represents the state after the *previous* player moved.
    // So the move we just picked is made by the *current* player (opponent of node->colour).
    char childColour = getOpponent(node->colour);
    
    // Convert index back to col/row
    int col = moveIndex % BOARD_SIZE;
    int row = moveIndex / BOARD_SIZE;

    // 4. Update the board state with this new move.
    board.set(col, row, childColour);
    
    // 5. Create a new child node representing this new board state.
    auto child = make_unique<Node>(col, row, childColour, node, board);
    Node* childPtr = child.get();

    // 6. Add this new child to the current node's list of children.
    node->children.push_back(std::move(child));
    
    // 7. Return the raw pointer to the newly created child so we can run a simulation from it.
    return childPtr;
}

MCTS::SimulationResult MCTS::simulate(Bitboard board, char turnColour) 
{
    SimulationResult result;
    
    // Optimized Simulation: Avoid vector allocation
    // We use a fixed array of indices 0..120 and shuffle it
    int moves[NUM_TILES];
    int movesCount = 0;
    
    // Populate valid moves
    for (int i = 0; i < NUM_TILES; ++i) 
    {
        int col = i % BOARD_SIZE;
        int row = i / BOARD_SIZE;
        if (!board.isOccupied(col, row)) 
        {
            moves[movesCount++] = i;
        }
    }
    
    // Random rollout
    while (true) 
    {
        if (board.checkWinRed()) 
        {
            result.winner = 'R';
            return result;
        }
        if (board.checkWinBlue()) 
        {
            result.winner = 'B';
            return result;
        }

        if (movesCount == 0) 
        {
            break; // Draw edge case
        }

        // Pick random move using FastRNG
        // Swap-remove strategy
        int index = rng.range(movesCount); 
        int moveIndex = moves[index];
        
        // Remove selected move by swapping with the last available move
        moves[index] = moves[--movesCount];
        
        int col = moveIndex % BOARD_SIZE;
        int row = moveIndex / BOARD_SIZE;
        
        board.set(col, row, turnColour);
        
        if (turnColour == 'R') 
        {
            result.redMoves.set(col, row, 'R');
        } 
        else 
        {
            result.blueMoves.set(col, row, 'B');
        }
        
        turnColour = getOpponent(turnColour);
    }
    result.winner = '0';
    return result;
}

void MCTS::backpropagate(Node* node, const SimulationResult& result) 
{
    // Walk up the tree from the leaf node to the root.
    while (node != nullptr) 
    {
        node->visits++;
        
        // UCT Update
        if (node->colour == result.winner) 
        {
            node->wins++;
        }
        
        // RAVE Update
        // Iterate over all children of the current node to update their AMAF stats
        // RAVE Update
        // Iterate over all children of the current node to update their AMAF stats
        char childColour = getOpponent(node->colour);
        const Bitboard& movesToCheck = (childColour == 'R') ? result.redMoves : result.blueMoves;
        
        for (const auto& childPtr : node->children) 
        {
            Node* child = childPtr.get();
            // Check if child's move appears in the simulation (O(1) check)
            if (movesToCheck.isOccupied(child->moveColumn, child->moveRow)) 
            {
                child->raveVisits++;
                if (childColour == result.winner) 
                {
                    child->raveWins++;
                }
            }
        }
        
        // Move up to the parent
        node = node->parent;
    }
}
