# LibTorch Configuration
TORCH_PATH = libtorch
TORCH_INCLUDES = -I$(TORCH_PATH)/include -I$(TORCH_PATH)/include/torch/csrc/api/include
TORCH_LIBS = -L$(TORCH_PATH)/lib -ltorch -ltorch_cpu -lc10 
# macOS might need -lc10 or just -ltorch depending on distribution.
# Linux usually needs: -ltorch -ltorch_cpu -lc10 -Wl,-rpath,$(TORCH_PATH)/lib
mac_RPATH = -Wl,-rpath,@loader_path/libtorch/lib

CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra $(TORCH_INCLUDES)
LDFLAGS = $(TORCH_LIBS) $(mac_RPATH)

TARGET = CppAgent
SRCS = CppAgent.cpp MCTS.cpp HexAgent.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET) Experiment ExperimentBase

Experiment: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -o Experiment Experiment.cpp MCTS.cpp

ExperimentBase: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -DNO_POOL -DNO_TT -o ExperimentBase Experiment.cpp MCTS.cpp

ExperimentNoTT: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -DNO_TT -o ExperimentNoTT Experiment.cpp MCTS.cpp
