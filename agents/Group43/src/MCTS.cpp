#include "MCTS.h"
#include <cstdlib>

using namespace std;
#include <random>

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

uint64_t MCTS::ZobristHasher::getHash(const Bitboard &board, char currentTurn)
{
    uint64_t h = 0;
    for (int i = 0; i < 121; ++i)
    {
        int col = i % 11;
        int row = i / 11;
        if (board.isOccupied(col, row))
        {
            char p = board.get(col, row);
            if (p == 'R')
                h ^= table[i][0];
            else if (p == 'B')
                h ^= table[i][1];
        }
    }
    if (currentTurn == 'R')
        h ^= turn[0];
    else
        h ^= turn[1];
    return h;
}

uint64_t MCTS::ZobristHasher::updateHash(uint64_t currentHash, int col, int row, char player)
{
    int index = row * 11 + col;
    // XOR in the new piece
    if (player == 'R')
        currentHash ^= table[index][0];
    else
        currentHash ^= table[index][1];

    // Flip turn
    currentHash ^= turn[0];
    currentHash ^= turn[1];

    return currentHash;
}

MCTS::SearchResult MCTS::runSearch(int timeLimitMs)
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
    Node *root = nodePool.alloc(-1, -1, getOpponent(myColour), nullptr, rootBoard, rootHash);
#else
    // UNOPTIMIZED: Use standard new (slower system call, fragmentation)
    Node *root = new Node(-1, -1, getOpponent(myColour), nullptr, rootBoard, rootHash);
#endif

    // Check TT for root
#ifndef NO_TT
    double wins;
    int visits;
    if (tt.lookup(rootHash, wins, visits))
    {
        root->wins = wins;
        root->visits = visits;
    }
#endif

    int iterations = 0;
    while (true)
    {
        // Check time every 1024 iterations to minimize overhead
        if ((iterations & 1023) == 0)
        {
            auto currentTime = chrono::high_resolution_clock::now();
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
            if (elapsed >= timeLimitMs)
            {
                break;
            }
        }

        Bitboard simulationBoard = rootBoard;

        // 1. Selection
        Node *leaf = select(root, simulationBoard);

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
        
        iterations++;
    }

    // Return best move (child with most visits)
    Node *bestChild = nullptr;
    int maxVisits = -1;

    for (const auto &childPtr : root->children)
    {
        Node *child = childPtr;
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
        return {move, iterations};
    }

    // Fallback if no search happened (should not happen)
#ifdef NO_POOL
    delete root;
#endif
    return {{-1, -1}, iterations};
}

Node *MCTS::select(Node *node, Bitboard &board)
{
    while (node->isFullyExpanded() && !node->children.empty())
    {
        node = node->bestChild(explorationConstant, raveConstant);
        board.set(node->moveColumn, node->moveRow, node->colour);
    }
    return node;
}

Node *MCTS::expand(Node *node, Bitboard &board)
{
    // Safety check (should be caught by isFullyExpanded, but good to have)
    if (node->remainingMoves == 0)
        return nullptr;

    // Bitwise Move Generation
    // 1. Calculate all occupied tiles (Red OR Blue)
    bitset<NUM_TILES> occupied = board.red | board.blue;

    // 2. Candidates are: (NOT Occupied) AND (NOT Already Expanded)
    bitset<NUM_TILES> candidates = ~occupied & ~node->expandedMoves;

    // 3. Pick a random valid move from the candidates
    // We want the k-th set bit, where k is a random number between 0 and candidate_count.
    int count = candidates.count();
    if (count == 0)
        return nullptr; // Should not happen if remainingMoves > 0

    // Pick a random index 'k'
    int pick = rng.range(count);

    int moveIndex = -1;
    int currentBit = 0;

    // Fast loop to find the k-th set bit
    // (This loops 121 times max, but usually much less. Average case is fast.)
    for (int i = 0; i < NUM_TILES; ++i)
    {
        if (candidates.test(i))
        {
            if (currentBit == pick)
            {
                moveIndex = i;
                break;
            }
            currentBit++;
        }
    }

    // 4. Update the Node state
    node->expandedMoves.set(moveIndex); // Mark this move as done
    node->remainingMoves--;             // Decrease count of moves left

    // 5. Standard Child Creation Logic (Same as before)
    // Determine who moves next (opponent of the node's storer)
    char childColour = getOpponent(node->colour);

    int col = moveIndex % BOARD_SIZE;
    int row = moveIndex / BOARD_SIZE;

    // Update board
    board.set(col, row, childColour);

    // Hash update
    uint64_t newHash = hasher.updateHash(node->hash, col, row, childColour);

#ifndef NO_POOL
    Node *child = nodePool.alloc(col, row, childColour, node, board, newHash);
#else
    Node *child = new Node(col, row, childColour, node, board, newHash);
#endif

#ifndef NO_TT
    double wins;
    int visits;
    if (tt.lookup(newHash, wins, visits))
    {
        child->wins = wins;
        child->visits = visits;
    }
#endif

    node->children.push_back(child);
    return child;
}

MCTS::SimulationResult MCTS::simulate(Bitboard board, char turnColour)
{
    SimulationResult result;

    // 1. Identify empty tiles (candidate moves)
    int moves[NUM_TILES];
    int movesCount = 0;

    for (int i = 0; i < NUM_TILES; ++i)
    {
        int col = i % BOARD_SIZE;
        int row = i / BOARD_SIZE;
        if (!board.isOccupied(col, row))
        {
            moves[movesCount++] = i;
        }
    }

    // 2. Play random moves untill the board is full
    // We do NOT check for a winner here. Speed is the only goal.
    while (movesCount > 0)
    {
        // Fast random pick
        int index = rng.range(movesCount);
        int moveIndex = moves[index];

        // Remove selected move (swap with end)
        moves[index] = moves[--movesCount];

        int col = moveIndex % BOARD_SIZE;
        int row = moveIndex / BOARD_SIZE;

        // Update local board
        board.set(col, row, turnColour);

        // Track moves for RAVE
        if (turnColour == 'R')
            result.redMoves.set(col, row, 'R');
        else
            result.blueMoves.set(col, row, 'B');

        // Flip turn
        turnColour = getOpponent(turnColour);
    }

    // 3. Check winner ONCE at the end
    // Since Hex has no draws, if Red hasn't won, Blue MUST have won.
    if (board.checkWinRed())
    {
        result.winner = 'R';
    }
    else
    {
        result.winner = 'B';
    }

    return result;
}

void MCTS::backpropagate(Node *node, const SimulationResult &result)
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
        const Bitboard &movesToCheck = (childColour == 'R') ? result.redMoves : result.blueMoves;

        for (const auto &childPtr : node->children)
        {
            Node *child = childPtr;
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
