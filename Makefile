CXX = zig c++
CXXFLAGS = -std=c++17 -Wall -g -I./lib
LDFLAGS =

SRC = main.cpp
TARGET = tiled2gslib

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

clear: clean

.PHONY: all build test clean
