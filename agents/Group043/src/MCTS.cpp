#include "MCTS.h"
#include "PatternWeights.h"
#include <cstdlib>
#include <random>

using namespace std;

// Static member definitions
MCTS::TranspositionTable MCTS::tt;
MCTS::ZobristHasher MCTS::hasher;

// Global node counter for benchmarking
long long g_nodeCount = 0;
int MCTS::neighborLUT[NUM_TILES][6];

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
    // OPTIMIZATION: Loop 0-120 directly using index overloads
    for (int i = 0; i < NUM_TILES; ++i)
    {
        if (board.isOccupied(i))
        {
            char p = board.get(i); // Requires the new get(index) overload
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

// OPTIMIZATION: Take index directly (avoiding row * 11 + col)
uint64_t MCTS::ZobristHasher::updateHash(uint64_t currentHash, int index, char player)
{
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

    // Initialize root node with Zobrist hash
    uint64_t rootHash = hasher.getHash(rootBoard, getOpponent(myColour));
    
    // Root Creation
    Node *root = new Node(255, getOpponent(myColour), nullptr, rootBoard, rootHash);

    // Check TT for root
    double wins;
    int visits;
    if (tt.lookup(rootHash, wins, visits))
    {
        root->wins = wins;
        root->visits = visits;
    }

    int iterations = 0;
    while (true)
    {
        // Check time every 64 iterations to minimize overhead
        // 1024 was too infrequent for heavy rollouts, leading to timeouts.
        if ((iterations & 63) == 0)
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
        g_nodeCount++;
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

    // Prepare result
    pair<int, int> resultMove = {-1, -1};

    // Return best move logic
    if (bestChild)
    {
        // Convert index back to coordinates ONLY at the very end
        int idx = bestChild->moveIndex;
        resultMove = {idx % BOARD_SIZE, idx / BOARD_SIZE};
    }

    delete root;
    return {resultMove, iterations};
}

Node *MCTS::select(Node *node, Bitboard &board)
{
    while (node->isFullyExpanded() && !node->children.empty())
    {
        node = node->bestChild(explorationConstant, raveConstant);
        // OPTIMIZATION: Use index overload directly
        board.set(node->moveIndex, node->colour);
    }
    return node;
}

Node *MCTS::expand(Node *node, Bitboard &board)
{
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

    // Update board
    board.set(moveIndex, childColour);

    // Hash update
    uint64_t newHash = hasher.updateHash(node->hash, moveIndex, childColour);

    Node *child = new Node(moveIndex, childColour, node, board, newHash);
    


    double wins;
    int visits;
    if (tt.lookup(newHash, wins, visits))
    {
        child->wins = wins;
        child->visits = visits;
    }

    node->children.push_back(child);
    return child;
}

void MCTS::initTables() {
    // Neighbor offsets for 11x11 board (Clockwise: TL, TR, R, BR, BL, L)
    const int offsets[6] = {-11, -10, 1, 11, 10, -1};

    for (int i = 0; i < NUM_TILES; ++i) {
        int r = i / 11;
        int c = i % 11;
        
        for (int k = 0; k < 6; ++k) {
            int nIdx = i + offsets[k];
            int nR = nIdx / 11;
            int nC = nIdx % 11;
            
            // Check bounds and row/col wrapping validity
            // Valid if inside 0-120 AND dist(row) <= 1 AND dist(col) <= 1
            if (nIdx >= 0 && nIdx < NUM_TILES && abs(nR - r) <= 1 && abs(nC - c) <= 1) {
                // Specific Hex wrap check: 
                // e.g. moving Right (k=2) shouldn't wrap to next row's start
                // The simple row/col distance check above covers most cases, 
                // but let's be explicit based on your original logic:
                bool valid = true;
                if (k == 2 && nC < c) valid = false; // Wrapped Right -> Left
                if (k == 5 && nC > c) valid = false; // Wrapped Left -> Right
                // (Other diagonals are handled by array bounds mostly, but row check is safer)
                
                neighborLUT[i][k] = valid ? nIdx : -1;
            } else {
                neighborLUT[i][k] = -1; // Wall
            }
        }
    }
}

// Define the helper as part of MCTS class
int MCTS::getPatternIndexFast(const Bitboard &board, int moveIndex, char myChar) {
    int patternIdx = 0;
    for (int k = 0; k < 6; ++k) {
        // Now can access private neighborLUT
        int nIdx = MCTS::neighborLUT[moveIndex][k];
        int code = 0; 

        if (nIdx == -1) {
            code = 2; // Wall
        } else {
            char p = board.get(nIdx); 
            if (p == '0') code = 0;
            else if (p == myChar) code = 1;
            else code = 2;
        }
        patternIdx |= (code << (k * 2));
    }
    return patternIdx;
}

// Updated Simulate function
MCTS::SimulationResult MCTS::simulate(Bitboard board, char turnColour)
{
    SimulationResult result;
    int moves[NUM_TILES];
    int movesCount = 0;

    for (int i = 0; i < NUM_TILES; ++i) {
        if (!board.isOccupied(i)) {
            moves[movesCount++] = i;
        }
    }

    const int MAX_ATTEMPTS = 4; 
    const uint32_t MAX_WEIGHT = 255; // Use unsigned here

    while (movesCount > 0)
    {
        int bestMoveIdx = -1;
        int selectedMove = -1;

        for (int k = 0; k < MAX_ATTEMPTS; ++k) {
            int rIdx = rng.range(movesCount);
            int candidate = moves[rIdx];

            int pattern = getPatternIndexFast(board, candidate, turnColour);
            
            // FIX: Read into unsigned variable. 
            // Since PATTERN_WEIGHTS is 'int', this casts implicitly.
            uint32_t weight = PATTERN_WEIGHTS[pattern]; 

            // Now comparison is uint32_t vs uint32_t (No warning)
            if (rng.range(MAX_WEIGHT) < weight) {
                bestMoveIdx = rIdx;
                selectedMove = candidate;
                break;
            }
            
            bestMoveIdx = rIdx;
            selectedMove = candidate;
        }

        moves[bestMoveIdx] = moves[--movesCount];
        board.set(selectedMove, turnColour);

        if (turnColour == 'R') result.redMoves.set(selectedMove, 'R');
        else result.blueMoves.set(selectedMove, 'B');

        turnColour = getOpponent(turnColour);
    }

    result.winner = board.checkWinRed() ? 'R' : 'B';
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
        tt.store(node->hash, node->wins, node->visits);

        // RAVE Update: Update AMAF stats for all children
        char childColour = getOpponent(node->colour);
        const Bitboard &movesToCheck = (childColour == 'R') ? result.redMoves : result.blueMoves;

        for (const auto &childPtr : node->children)
        {
            Node *child = childPtr;
            // OPTIMIZATION: Check RAVE using index directly
            if (movesToCheck.isOccupied(child->moveIndex))
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
