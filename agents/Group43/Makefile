CXX = g++

# Directories
SRC_DIR = src
EXP_DIR = experiments
BIN_DIR = bin

# Compiler Flags
# -I$(SRC_DIR): Tells compiler to look for headers (.h) in the src folder
# -march=native: Optimizes for your specific CPU
# -flto: Link Time Optimization
# -DNDEBUG: Disables asserts (Critical for speed)
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