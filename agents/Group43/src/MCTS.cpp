#include "MCTS.h"
#include <cstdlib>

using namespace std;
#include <random>
#include <deque>

// Static member definitions
MCTS::TranspositionTable MCTS::tt;
MCTS::ZobristHasher MCTS::hasher;
MCTS::NodePool MCTS::nodePool;

// Global node counter for benchmarking
long long g_nodeCount = 0;

MCTS::ZobristHasher::ZobristHasher() 
{
    mt19937_64 rng(12345);
    for (int i = 0; i < 121; ++i) 
    {
        table[i][0] = rng(); // Red
        table[i][1] = rng(); // Blue
    }
    turn[0] = rng(); // Red's turn
    turn[1] = rng(); // Blue's turn
}

uint64_t MCTS::ZobristHasher::getHash(const Bitboard& board, char currentTurn) 
{
    uint64_t h = 0;
    for (int i = 0; i < 121; ++i) 
    {
        int col = i % 11;
        int row = i / 11;
        if (board.isOccupied(col, row)) 
        {
            char p = board.get(col, row);
            if (p == 'R') h ^= table[i][0];
            else if (p == 'B') h ^= table[i][1];
        }
    }
    if (currentTurn == 'R') h ^= turn[0];
    else h ^= turn[1];
    return h;
}

uint64_t MCTS::ZobristHasher::updateHash(uint64_t currentHash, int col, int row, char player) 
{
    int index = row * 11 + col;
    // XOR in the new piece
    if (player == 'R') currentHash ^= table[index][0];
    else currentHash ^= table[index][1];
    
    // Flip turn
    currentHash ^= turn[0];
    currentHash ^= turn[1];
    
    return currentHash;
}


pair<int, int> MCTS::runSearch(int timeLimitMs) 
{
    auto startTime = chrono::high_resolution_clock::now();
    
    // Reset the memory pool for the new search
#ifndef NO_POOL
    nodePool.reset();
#endif
    
    // Initialize root node with Zobrist hash
    uint64_t rootHash = hasher.getHash(rootBoard, getOpponent(myColour));
#ifndef NO_POOL
    // OPTIMIZED: Use Memory Pool (O(1) allocation, better cache locality)
    Node* root = nodePool.alloc(-1, -1, getOpponent(myColour), nullptr, rootBoard, rootHash);
#else
    // UNOPTIMIZED: Use standard new (slower system call, fragmentation)
    Node* root = new Node(-1, -1, getOpponent(myColour), nullptr, rootBoard, rootHash);
#endif

    // Check TT for root
#ifndef NO_TT
    double wins; int visits;
    if (tt.lookup(rootHash, wins, visits)) 
    {
        root->wins = wins;
        root->visits = visits;
    }
#endif

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
        Node* leaf = select(root, simulationBoard);

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
        Node* child = childPtr;
        if (child->visits > maxVisits) 
        {
            maxVisits = child->visits;
            bestChild = child;
        }
    }

    if (bestChild) 
    {
        pair<int, int> move = {bestChild->moveColumn, bestChild->moveRow};
#ifdef NO_POOL
        delete root;
#endif
        return move;
    }
    
    // Fallback if no search happened (should not happen)
#ifdef NO_POOL
    delete root;
#endif
    return {-1, -1};
}

Node* MCTS::select(Node* node, Bitboard& board) 
{
    while (node->isFullyExpanded() && !node->children.empty()) 
    {
        node = node->bestChild(explorationConstant, raveConstant);
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
    
    // Create child node using Memory Pool
    uint64_t newHash = hasher.updateHash(node->hash, col, row, childColour);
#ifndef NO_POOL
    // OPTIMIZED: Use Memory Pool
    Node* child = nodePool.alloc(col, row, childColour, node, board, newHash);
#else
    // UNOPTIMIZED: Use standard new
    Node* child = new Node(col, row, childColour, node, board, newHash);
#endif
    
    // Check TT for initialization
#ifndef NO_TT
    // OPTIMIZED: Check Transposition Table for existing data
    double wins; int visits;
    if (tt.lookup(newHash, wins, visits)) 
    {
        child->wins = wins;
        child->visits = visits;
    }
#endif

    // 6. Add this new child to the current node's list of children.
    node->children.push_back(child);
    
    // 7. Return the raw pointer to the newly created child so we can run a simulation from it.
    return child;
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
        
        // Update TT
#ifndef NO_TT
        tt.store(node->hash, node->wins, node->visits);
#endif
        
        // RAVE Update: Update AMAF stats for all children
        char childColour = getOpponent(node->colour);
        const Bitboard& movesToCheck = (childColour == 'R') ? result.redMoves : result.blueMoves;
        
        for (const auto& childPtr : node->children) 
        {
            Node* child = childPtr;
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
