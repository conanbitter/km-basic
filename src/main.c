#include <stdio.h>

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"
#include "memblock.h"

#include "exptree.h"
#include "bytecode.h"

#define BUFFER_SIZE (1024)

char buffer[BUFFER_SIZE];

int main() {
#ifdef NDEBUG
    printf("Not debug\n");
#else
    printf("Debug\n");
#endif
    /*printf("ExptBinop: %zu\n", sizeof(ExptBinop));
    printf("ExptUnop: %zu\n", sizeof(ExptUnop));
    printf("ExptConst: %zu\n", sizeof(ExptConst));
    printf("ExptLoad: %zu\n", sizeof(ExptLoad));
    printf("ExptNode: %zu\n", sizeof(ExptNode));

    ExptBinop test = { 0 };
    test.node_type = 2;
    test.opcode = 28;
    printf("\nByte: %d\n", *((uint8_t*)&test));
    printf("ExptBinop type: %d\n", test.node_type);
    printf("ExptUnop type: %d\n", ((ExptUnop*)&test)->node_type);
    printf("ExptConst type: %d\n", ((ExptConst*)&test)->node_type);
    printf("ExptLoad type: %d\n", ((ExptLoad*)&test)->node_type);
    printf("ExptNode type: %d\n", ((ExptNode*)&test)->node_type);*/
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
    bc_init();
    parse(buffer, buffer + BUFFER_SIZE);
    close_file();
    return 0;
}