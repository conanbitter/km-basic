#include "common.h"
#include "lexer.h"
#include "parser.h"

#include "stdbool.h"

typedef struct DictHeader {
    struct DictHeader* prev;
    uint32_t hash;
} DictHeader;

typedef enum DataType {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING
} DataType;

typedef struct ExprResult {
    DataType data_type;
    bool is_literal;
    union {
        KmInt int_value;
        KmFloat float_value;
        size_t length;
    };
} ExprResult;

static char* buffer;
static const char* buffer_end;
static char* work_data;

#define NEXT next_token(work_data, buffer_end)

static void expect(TokenType token_type) {
    if (token.token_type == token_type) {
        NEXT;
    } else {
        printf("[%d:%d] ERROR: expected '%s', got '%s'", token.line, token.col, TOKEN_NAMES[token_type], TOKEN_NAMES[token.token_type]);
        exit(1);
    }
}

static void unexpected() {
    printf("[%d:%d] ERROR: '%s' is unexpected", token.line, token.col, TOKEN_NAMES[token.token_type]);
    exit(1);
}

static ExprResult expr() {
    ExprResult result;
    switch (token.token_type)
    {
    case TOKEN_INTLIT:
        result.data_type = TYPE_INT;
        result.is_literal = true;
        result.int_value = token.int_value;
        NEXT;
        return result;

    case TOKEN_FLOATLIT:
        result.data_type = TYPE_FLOAT;
        result.is_literal = true;
        result.float_value = token.float_value;
        NEXT;
        return result;

    case TOKEN_STRLIT:
        result.data_type = TYPE_STRING;
        result.is_literal = true;
        result.length = token.length;
        NEXT;
        return result;

    default:
        unexpected();
        break;
    }
}

void parse(char* _buffer, const char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    expr();
}

