CC = gcc
CXX = g++
CFLAGS = -Wall -Iinclude -I../deps/gumbo-parser/src -std=c99 -Wno-unused-function
CXXFLAGS = -std=c++17 -Wall -Iinclude -I../deps/gumbo-parser/src

GUMBO_SRC = ../deps/gumbo-parser/src/attribute.c \
            ../deps/gumbo-parser/src/char_ref.c \
            ../deps/gumbo-parser/src/char_ref_gperf.c \
            ../deps/gumbo-parser/src/error.c \
            ../deps/gumbo-parser/src/parser.c \
            ../deps/gumbo-parser/src/string_buffer.c \
            ../deps/gumbo-parser/src/string_piece.c \
            ../deps/gumbo-parser/src/tag.c \
            ../deps/gumbo-parser/src/tokenizer.c \
            ../deps/gumbo-parser/src/utf8.c \
            ../deps/gumbo-parser/src/util.c \
            ../deps/gumbo-parser/src/vector.c
GUMBO_OBJ = $(GUMBO_SRC:.c=.o)

SRC = src/html_parser.cpp src/html_extractor.cpp
OBJ = $(SRC:.cpp=.o)

SAMPLE_SRC = samples/sample.cpp
TARGET = bin/parser

.PHONY: all clean run

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJ) $(GUMBO_OBJ) $(SAMPLE_SRC)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf bin/ build/ $(OBJ) $(GUMBO_OBJ)

run: all
	./$(TARGET) ../samples/gmail.eml
