.PHONY: clean distclean default test

CC=g++
CFLAGS=-Wall

default: dana

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

lexer.o: lexer.cpp parser.hpp ast.hpp

parser.hpp parser.cpp: parser.y
	bison -dv -o parser.cpp parser.y

parser.o: parser.cpp ast.hpp

ast.o: ast.cpp ast.hpp

dana: lexer.o parser.o ast.o
	$(CC) $(CFLAGS) -o dana $^ -lfl

test:
	@echo "Running tests on all .dana files in ../Dana/ directory..."
	@for file in ../Dana/*.dana; do \
		if [ -f "$$file" ]; then \
			echo "\nTesting: $$file"; \
			./dana < "$$file"; \
		fi \
	done

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o *~

distclean: clean
	$(RM) dana
