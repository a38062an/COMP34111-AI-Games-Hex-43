CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra

TARGET = CppAgent
SRCS = CppAgent.cpp MCTS.cpp HexAgent.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
