# Constants
WEIGHT_CRITICAL_SAVE = 255  # Saving a bridge or edge win
WEIGHT_URGENT_BLOCK  = 100  # Blocking opponent expansion
WEIGHT_SOLID_CONN    = 40   # Connecting split groups (Good)
WEIGHT_DEFAULT       = 5    # Standard move in open space
WEIGHT_CLUMP         = 1    # BAD: Touching own stone without purpose

def get_weight(neighbors):
    weight = WEIGHT_DEFAULT

    # Flags
    has_my_bridge = False
    has_opp_bridge = False
    touches_target_wall = False

    for i in range(6):
        n_curr = neighbors[i]
        n_next = neighbors[(i+1)%6]
        n_gap  = neighbors[(i+2)%6]

        # 1. Detect Bridge Shapes (Vital Points)
        # Opponent Bridge: 2-0-2 (Opponent - Empty - Opponent)
        if n_curr == 2 and n_next == 0 and n_gap == 2:
            has_opp_bridge = True

        # My Bridge: 1-0-1 (Me - Empty - Me)
        # Completing a bridge is usually better than a solid connection
        if n_curr == 1 and n_next == 0 and n_gap == 1:
            has_my_bridge = True

        # 2. Edge Connectivity (Assumes '3' is ONLY the Target Wall)
        if (n_curr == 1 and n_next == 3) or (n_curr == 3 and n_next == 1):
            touches_target_wall = True

        # 3. Simple blocking (Adjacency)
        if n_curr == 2:
            weight = max(weight, WEIGHT_URGENT_BLOCK)

    # --- FINAL DECISION LOGIC ---

    # Highest Priority: Connecting to a Target Wall
    if touches_target_wall:
        return 200 # Critical, but maybe not 255 (leave room for unkillable threats)

    # High Priority: Forming my own Bridge (Travel)
    if has_my_bridge:
        return 180 # Boosted significantly from 40

    # Medium-High Priority: Intruder into Opponent Bridge
    if has_opp_bridge:
        return 150 # Good to force response, but lower than building my own win

    # Medium: Solid Connection (1-1 Adjacency)
    # Scan again for simple adjacency if no bridge found
    is_solid = False
    for i in range(6):
        if neighbors[i] == 1 and neighbors[(i+1)%6] == 1:
            is_solid = True
            break
    if is_solid:
        return 40 # Solid connection is okay, but slow

    # "Clumping" Logic (Downgrade bad moves)
    # If touching my stone but NOT forming a bridge/solid/wall-connect
    touches_me = any(n == 1 for n in neighbors)
    touches_opp = any(n == 2 for n in neighbors)

    if touches_me and not touches_opp and not has_my_bridge and not is_solid:
        return WEIGHT_CLUMP # Penalty for useless contact

    return weight

def generate_table():
    print("// HexPatternWeights.h - Auto-generated")
    print("// Size: 4096 entries (12-bit index)")
    print("const int PATTERN_WEIGHTS[4096] = {")

    output_buffer = []

    # Iterate 0 to 4095
    for index in range(4096):
        neighbors = []
        temp_idx = index

        # Decode the 12-bit integer back into 6 neighbors (2 bits each)
        for _ in range(6):
            neighbors.append(temp_idx & 3) # Get last 2 bits
            temp_idx >>= 2                 # Shift right

        w = get_weight(neighbors)
        output_buffer.append(str(w))

    # Formatting for C++ array
    print(", ".join(output_buffer))
    print("};")

if __name__ == "__main__":
    generate_table()
