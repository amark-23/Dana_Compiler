.PHONY: clean distclean default test

CXX=g++
CXXFLAGS= -Wall
TEST_DIR= ../compilersNTUA/dana
DANA_BIN= ./dana

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
	@echo "==========================================="
	@echo "Which test mode do you want to run?"
	@echo "  1) Sunny day (success scenarios)"
	@echo "  2) Rainy day (error/erroneous scenarios)"
	@echo "  3) All tests"
	@echo "  (Enter 1, 2, or 3, or press Ctrl+C to abort)"
	@read mode; \
	case "$$mode" in \
		1) $(MAKE) test-sunny ;; \
		2) $(MAKE) test-rainy ;; \
		3) $(MAKE) test-sunny; $(MAKE) test-rainy ;; \
		*) echo "Invalid option '$$mode'. Aborting."; exit 1 ;; \
	esac

# Target for successful (sunny day) programs
test-sunny:
	@echo "\n==========================================="
	@echo "  Running SUNNY DAY tests (success expected)..."
	@echo "==========================================="
	@for file in $(TEST_DIR)/programs/*.dana; do \
		if [ -f "$$file" ]; then \
			echo "\nTesting success: $$file"; \
			$(DANA_BIN) < "$$file"; \
		fi \
	done

# Target for erroneous (rainy day) programs
test-rainy:
	@echo "\n==========================================="
	@echo "  Running RAINY DAY tests (errors expected)..."
	@echo "==========================================="
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
