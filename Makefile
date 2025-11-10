.PHONY: clean distclean default test

GREEN=\033[0;32m
RED=\033[0;31m
YELLOW=\033[0;33m
NC=\033[0m

CXX=g++
CXXFLAGS= -Wall
TEST_DIR= ./compilersNTUA/dana
DANA_BIN= ./dana
PYTHON=python3
TEST_SCRIPT=test_runner.py

default: dana

dana: lexer.o parser.o ast.o symbol.o semantic.o runtime.o
	$(CXX) $(CXXFLAGS) -o dana $^ -lfl

lexer.o: lexer.cpp parser.hpp
parser.o: parser.cpp parser.hpp
ast.o: ast.cpp ast.hpp
symbol.o: symbol.cpp symbol.hpp
semantic.o: semantic.cpp
runtime.o: runtime.hpp runtime.cpp

lexer.cpp: lexer.l ast.hpp ast.cpp
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y ast.hpp ast.cpp
	bison -dv -o parser.cpp parser.y

test:
	@echo "\nWhich test mode do you want to run?"
	@echo "  1) Sunny day"
	@echo "  2) Rainy day"
	@echo "  3) All tests"
	@echo "  (Enter 1, 2, or 3, or press Ctrl+C to abort)"
	@read mode; \
	case "$$mode" in \
		1) $(MAKE) test-sunny ;; \
		2) $(MAKE) test-rainy ;; \
		3) $(MAKE) test-sunny; $(MAKE) test-rainy ;; \
		*) echo "Invalid option '$$mode'. Aborting."; exit 1 ;; \
	esac

test-sunny: dana
	@echo "\n============================"
	@echo "   Running SUNNY DAY tests"
	@echo "============================"
	@$(PYTHON) $(TEST_SCRIPT) $(TEST_DIR)/programs $(DANA_BIN)

test-rainy:
	@echo "\n============================"
	@echo "  Running RAINY DAY tests"
	@echo "============================"
	@for file in $(TEST_DIR)/programs-erroneous/*.dana; do \
		if [ -f "$$file" ]; then \
			echo "\nTesting erroneous: $$file"; \
			$(DANA_BIN) < "$$file"; \
		fi \
	done

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o *~

distclean: clean
	$(RM) dana
