CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g
LDFLAGS = -lGL -lGLU -lglut
TARGET = build/graph_display
HEADERS = src/graph.hpp src/graph_generator.hpp

# Default build target
all: $(TARGET)

$(TARGET): build/main.o
	$(CXX) $^ -o $@ $(LDFLAGS)

build/main.o: src/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all run clean
