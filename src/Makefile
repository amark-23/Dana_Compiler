# Minimal Makefile for building the Dana lexer/compiler driver

CC      = gcc
LEX     = flex
TARGET  = dana-compiler

# Source files
LEXER_SRC = lexer.l
MAIN_SRC  = main.c

# Generated files
LEXER_C   = lexer.c

# Object files
OBJS      = lexer.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

lexer.o: $(LEXER_C)
	$(CC) -c $(LEXER_C) -o lexer.o

$(LEXER_C): $(LEXER_SRC)
	$(LEX) -o $(LEXER_C) $(LEXER_SRC)

main.o: $(MAIN_SRC)
	$(CC) -c $(MAIN_SRC) -o main.o

clean:
	rm -f $(TARGET) $(OBJS) $(LEXER_C)
