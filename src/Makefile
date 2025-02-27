# Minimal Makefile for a Flex-based C++ lexer

# Our final executable
TARGET = lexer

# Source and generated files
LEXER_SRC = lexer.l
LEXER_OUT = lexer.cpp

# Compiler and flags
CXX = c++
CXXFLAGS = -std=c++11

all: $(TARGET)

# Rule to generate lexer.cpp from lexer.l
$(LEXER_OUT): $(LEXER_SRC)
	flex -o $(LEXER_OUT) $(LEXER_SRC)

# Rule to compile the generated lexer.cpp into our final executable
$(TARGET): $(LEXER_OUT)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(LEXER_OUT)

clean:
	rm -f $(TARGET) $(LEXER_OUT)
