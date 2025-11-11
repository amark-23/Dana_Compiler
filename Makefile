.PHONY: clean distclean default test test-sunny test-rainy

GREEN=\033[0;32m
RED=\033[0;31m
YELLOW=\033[0;33m
NC=\033[0m

CXX=g++
LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)
FILTERED_LLVM_CXXFLAGS = $(filter-out -fno-exceptions, $(LLVM_CXXFLAGS))
CXXFLAGS = -Wall -std=c++17 $(FILTERED_LLVM_CXXFLAGS)
LDFLAGS = $(shell llvm-config --ldflags)
LIBS = $(shell llvm-config --libs --system-libs) -lfl

TEST_DIR= ./compilersNTUA/dana
DANA_BIN= ./dana
PYTHON=python3
TEST_SCRIPT=test_runner.py

OBJS = lexer.o parser.o ast.o symbol.o semantic.o runtime.o codegen.o

default: dana

dana: $(OBJS)
	@echo "$(GREEN)Linking executable: dana...$(NC)"
	$(CXX) $(CXXFLAGS) -o dana $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	@echo "$(YELLOW)Compiling: $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

lexer.o: lexer.cpp parser.hpp ast.hpp runtime.hpp
parser.o: parser.cpp parser.hpp ast.hpp symbol.hpp
ast.o: ast.cpp ast.hpp symbol.hpp
symbol.o: symbol.cpp symbol.hpp ast.hpp
semantic.o: semantic.cpp ast.hpp symbol.hpp
runtime.o: runtime.cpp runtime.hpp ast.hpp symbol.hpp
codegen.o: codegen.cpp codegen.hpp ast.hpp symbol.hpp

lexer.cpp: lexer.l ast.hpp
	@echo "Running Flex..."
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y ast.hpp
	@echo "Running Bison..."
	bison -dv -o parser.cpp parser.y

test:
	@echo "\nWhich test mode do you want to run?"
	@echo " 	1) Sunny day"
	@echo " 	2) Rainy day"
	@echo " 	3) All tests"
	@echo " 	(Enter 1, 2, or 3, or press Ctrl+C to abort)"
	@read mode; \
	case "$$mode" in \
		1) $(MAKE) test-sunny ;; \
		2) $(MAKE) test-rainy ;; \
		3) $(MAKE) test-sunny; $(MAKE) test-rainy ;; \
		*) echo "Invalid option '$$mode'. Aborting."; exit 1 ;; \
	esac

test-sunny: dana
	@echo "\n============================"
	@echo " 	Running SUNNY DAY tests"
	@echo "============================"
	@$(PYTHON) $(TEST_SCRIPT) $(TEST_DIR)/programs $(DANA_BIN)

test-rainy:
	@echo "\n============================"
	@echo " 	Running RAINY DAY tests"
	@echo "============================"
	@for file in $(TEST_DIR)/programs-erroneous/*.dana; do \
		if [ -f "$$file" ]; then \
			echo "\nTesting erroneous: $$file"; \
			$(DANA_BIN) < "$$file"; \
		fi \
	done

clean:
	@echo "Cleaning up..."
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o *~
	$(RM) -r test_results

distclean: clean
	$(RM) dana