.PHONY: clean distclean default test

CXX=g++
CXXFLAGS= -Wall

default: dana

dana: lexer.o parser.o ast.o symbol.o semantic.o
	$(CXX) $(CXXFLAGS) -o dana $^ -lfl

lexer.o: lexer.cpp parser.hpp
parser.o: parser.cpp parser.hpp
ast.o: ast.cpp ast.hpp
symbol.o: symbol.cpp symbol.hpp
semantic.o: semantic.cpp

lexer.cpp: lexer.l ast.hpp ast.cpp
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y ast.hpp ast.cpp
	bison -dv -o parser.cpp parser.y

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
