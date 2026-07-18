CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude
LDFLAGS = -lgumbo

SRC = src/html_parser.cpp src/html_extractor.cpp
SAMPLE_SRC = samples/sample.cpp
TARGET = bin/parser

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) $(SAMPLE_SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf bin/ build/

run: all
	./$(TARGET) ../samples/gmail.eml
