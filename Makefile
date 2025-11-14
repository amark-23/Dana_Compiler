.PHONY: clean distclean default

GREEN=\033[0;32m
RED=\033[0;31m
YELLOW=\033[0;33m
NC=\033[0m

CXX=g++
LLVM_CXXFLAGS = $(shell llvm-config --cxxflags)
FILTERED_LLVM_CXXFLAGS = $(filter-out -fno-exceptions, $(LLVM_CXXFLAGS))
CXXFLAGS = -Wall -std=c++17 $(FILTERED_LLVM_CXXFLAGS)
LIBS = $(shell llvm-config --libs --system-libs all) -lfl
EXEC_NAME = dana
OBJS = lexer.o parser.o ast.o symbol.o semantic.o codegen.o
RUNTIME_LIB = runtime_lib.o
PYTHON_CORRECT_FILE = ./compilersNTUA/tests/test-correct.py
PYTHON_ERRONEOUS_FILE = ./compilersNTUA/tests/test-erroneous.py
TEST_CORRECT_DIR = ./compilersNTUA/dana/programs/
TEST_ERRONEOUS_DIR = ./compilersNTUA/dana/programs-erroneous/

default: $(EXEC_NAME) $(RUNTIME_LIB)

$(EXEC_NAME): $(OBJS)
	@echo "$(GREEN)Linking executable: $(EXEC_NAME)...$(NC)"
	$(CXX) $(CXXFLAGS) -o $(EXEC_NAME) $(OBJS) $(LDFLAGS) $(LIBS)

$(RUNTIME_LIB): runtime_lib.cpp
	@echo "$(YELLOW)Compiling runtime library: $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "$(YELLOW)Compiling: $<...$(NC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

lexer.o: lexer.cpp parser.hpp ast.hpp codegen.hpp
parser.o: parser.cpp parser.hpp ast.hpp symbol.hpp codegen.hpp
ast.o: ast.cpp ast.hpp symbol.hpp
symbol.o: symbol.cpp symbol.hpp ast.hpp
semantic.o: semantic.cpp ast.hpp symbol.hpp
codegen.o: codegen.cpp codegen.hpp ast.hpp symbol.hpp 

lexer.cpp: lexer.l ast.hpp
	@echo "Running Flex..."
	flex -s -o lexer.cpp lexer.l

parser.hpp parser.cpp: parser.y ast.hpp
	@echo "Running Bison..."
	bison -dv -o parser.cpp parser.y

clean:
	@echo "Cleaning up object files and generated sources..."
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output *.o *~
	$(RM) -r test_results

distclean: clean
	@echo "Cleaning up executable..."
	$(RM) $(EXEC_NAME)

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
	@$(PYTHON) $(PYTHON_CORRECT_FILE) dana ./dana $(TEST_CORRECT_DIR)

test-rainy:
	@echo "\n============================"
	@echo " 	Running RAINY DAY tests"
	@echo "============================"
	@$(PYTHON) $(PYTHON_ERRONEOUS_FILE) dana ./dana $(TEST_ERRONEOUS_DIR)