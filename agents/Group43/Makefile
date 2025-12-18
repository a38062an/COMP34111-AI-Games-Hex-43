# LibTorch Configuration
# Path relative to this Makefile (agents/Group43/)
TORCH_PATH = src/libtorch
TORCH_INCLUDES = -I$(TORCH_PATH)/include -I$(TORCH_PATH)/include/torch/csrc/api/include
TORCH_LIBS = -L$(TORCH_PATH)/lib -ltorch -ltorch_cpu -lc10 
# macOS RPATH: Binary is in bin/, libtorch is in src/libtorch/lib
# So from bin/ we go ../src/libtorch/lib
mac_RPATH = -Wl,-rpath,@loader_path/../src/libtorch/lib

CXX = g++

# Directories
SRC_DIR = src
EXP_DIR = benchmarks
BIN_DIR = bin

# Compiler Flags
# Added TORCH_INCLUDES
CXXFLAGS = -O3 -march=native -flto -DNDEBUG -std=c++17 -Wall -Wextra -Wno-unused-parameter -I$(SRC_DIR) $(TORCH_INCLUDES)

# The list of core logic files shared between the Agent and Experiments
AGENT_SRCS = $(SRC_DIR)/MCTS.cpp $(SRC_DIR)/HexAgent.cpp

# --- Main Targets ---

all: $(BIN_DIR)/CppAgent

# 1. The Competition Agent
$(BIN_DIR)/CppAgent: $(SRC_DIR)/CppAgent.cpp $(AGENT_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(TORCH_LIBS) $(mac_RPATH)

# 2. EXPERIMENTAL AGENT (Feature Branch Compilation)
# Run 'make dev' on your feature branch to generate this.
# It compiles the exact same source code (CppAgent.cpp) but saves it as 'DevAgent'.
$(BIN_DIR)/DevAgent: $(SRC_DIR)/CppAgent.cpp $(AGENT_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(TORCH_LIBS) $(mac_RPATH)

# --- Benchmarking Targets ---

# Speed Test (Simulations Per Second)
$(BIN_DIR)/SPS: $(EXP_DIR)/SPS.cpp $(AGENT_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(TORCH_LIBS) $(mac_RPATH)

benchmark: $(BIN_DIR)/SPS
	./$(BIN_DIR)/SPS

# --- Utilities ---

# Helper alias for building the feature agent
dev: $(BIN_DIR)/DevAgent
	@echo "Compiled feature branch to $(BIN_DIR)/DevAgent"

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean dev