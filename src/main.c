#include <stdio.h>

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"

char buffer[256];

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
    parse(buffer, buffer + 256);
    close_file();
    return 0;
}