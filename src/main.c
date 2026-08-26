#include <stdio.h>

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"

#define BUFFER_SIZE (1024)

char buffer[BUFFER_SIZE];

int main() {
    open_file("parsetest.bas");
    /*
        open_file("sample.bas");
        next_token(buffer, 256);
        while (token.token_type != TOKEN_ERROR && token.token_type != TOKEN_EOF) {
            print_token(buffer);
            printf("\n");
            next_token(buffer, 256);
        }
    */
    parse(buffer, buffer + BUFFER_SIZE);
    close_file();
    return 0;
}