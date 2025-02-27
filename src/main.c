#include <stdio.h>
#include <stdlib.h>

/* Flex automatically generates this prototype. */
extern int yylex();
/* By default, flex reads from 'yyin'. */
extern FILE *yyin;

int main(int argc, char *argv[])
{
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("fopen");
            return 1;
        }
    }

    /* Run the lexer until EOF. */
    yylex();

    /* Close file if it was opened. */
    if (argc > 1 && yyin) {
        fclose(yyin);
    }

    return 0;
}
