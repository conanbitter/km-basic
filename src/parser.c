#include "common.h"
#include "lexer.h"
#include "parser.h"

typedef struct DictHeader {
    struct DictHeader* prev;
    uint32_t hash;
} DictHeader;

static char* buffer;
static const char* buffer_end;

#define NEXT next_token(buffer+sizeof(DictHeader), buffer_end)

static void expect(TokenType token_type) {
    if (token.token_type == token_type) {
        NEXT;
    } else {
        printf("[%d:%d] ERROR: expected '%s', got '%s'", token.line, token.col, TOKEN_NAMES[token_type], TOKEN_NAMES[token.token_type]);
        exit(1);
    }
}

static void expr() {
    if (token.token_type == TOKEN_INTLIT) {
        printf("const int = %d, %d", token.int_value, sizeof(DictHeader));
    }
}

void parse(char* _buffer, const char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    NEXT;
    expr();
}

