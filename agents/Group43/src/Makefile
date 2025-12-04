CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra

TARGET = CppAgent
SRCS = CppAgent.cpp MCTS.cpp HexAgent.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) Experiment ExperimentBase

Experiment: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -o Experiment Experiment.cpp MCTS.cpp

ExperimentBase: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -DNO_POOL -DNO_TT -o ExperimentBase Experiment.cpp MCTS.cpp

ExperimentNoTT: Experiment.cpp MCTS.cpp
	$(CXX) $(CXXFLAGS) -DNO_TT -o ExperimentNoTT Experiment.cpp MCTS.cpp
