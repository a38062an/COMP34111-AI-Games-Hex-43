CXX = g++

# Directories
SRC_DIR = src
EXP_DIR = experiments
BIN_DIR = bin

# Compiler Flags

# Xeon Gold 5416S (Sapphire Rapids) Optimization: (COMMENTED OUT FOR NOW!)
# -march=sapphirerapids: Target Xeon 5416S instruction set (AVX-512, AMX).
# -mprefer-vector-width=512: Force full 512-bit vector registers for bitboards.
# -flto: Inline small functions across translation units.
# -DNDEBUG: Disable asserts to remove runtime checking overhead.
#CXXFLAGS = -O3 -march=sapphirerapids -mprefer-vector-width=512 -flto -DNDEBUG -std=c++17 -Wall -Wextra -I$(SRC_DIR)

# Standard Build
CXXFLAGS = -O3 -march=native -flto -DNDEBUG -std=c++17 -Wall -Wextra -I$(SRC_DIR)

# The list of core logic files shared between the Agent and Experiments
# (Note: Headers depend on these, but we compile the .cpp files)
AGENT_SRCS = $(SRC_DIR)/MCTS.cpp $(SRC_DIR)/HexAgent.cpp

# --- Main Targets ---

all: $(BIN_DIR)/CppAgent

# 1. The Competition Agent
# Combines CppAgent.cpp (Entry point) + Core Logic
$(BIN_DIR)/CppAgent: $(SRC_DIR)/CppAgent.cpp $(AGENT_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Benchmarking Targets ---

# Standard Experiment
$(BIN_DIR)/Experiment: $(EXP_DIR)/Experiment.cpp $(AGENT_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Utilities ---

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean