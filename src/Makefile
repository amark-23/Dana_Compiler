.PHONY: clean distclean default test

CC=gcc
CFLAGS=-Wall

default: dana

lexer.c: lexer.l
	flex -s -o lexer.c lexer.l

lexer.o: lexer.c parser.h

parser.h parser.c: parser.y
	bison -dv -o parser.c parser.y

parser.o: parser.c

dana: lexer.o parser.o
	$(CC) $(CFLAGS) -o dana $^ -lfl

test:
	@echo "Running tests on all .dana files in ../Dana/ directory..."
	@for file in ../Dana/*.dana; do \
		if [ -f "$$file" ]; then \
			echo "Testing: $$file"; \
			./dana < "$$file"; \
			echo ""; \
		fi \
	done

clean:
	$(RM) lexer.c parser.c parser.h parser.output *.o *~

distclean: clean
	$(RM) dana
