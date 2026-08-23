#include "common.h"
#include "lexer.h"
#include "parser.h"

#include <stdbool.h>
#include <stdalign.h>

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
        char* pointer;
    };
} ExprResult;

static char* buffer;
static const char* buffer_end;
static char* work_data;
static DictHeader* prev = NULL;

#define NEXT next_token(work_data, buffer_end)
#define LENGTH_MASK (1<<15-1)
#define STRING_FLAG (1<<15)

const uintptr_t ptr_alignment = _Alignof(KmInt);

static char* align_ptr(char* ptr) {
    uintptr_t intptr = (uintptr_t)ptr;
    uintptr_t result = (intptr + ptr_alignment - 1) & ~(ptr_alignment - 1);
    return (char*)result;
}

static uint32_t get_hash(const char* text, size_t length, bool is_string) {
    uint32_t hash = 0;
    if (is_string) hash |= STRING_FLAG;            // 1  bit  - string literal flag
    hash |= (length & LENGTH_MASK) << 16;          // 15 bits - text length
    hash |= (uint32_t)(*text) << 8;                // 8  bits - first character
    hash |= (uint32_t)(*(text + length - 1)) << 8; // 8  bits - second character
}

static void emplace_string(size_t length) {
    DictHeader* current = (DictHeader*)buffer;
    current->prev = prev;
    current->hash = get_hash(work_data, length, true);
    prev = current;
    buffer = align_ptr(buffer + sizeof(DictHeader) + length);
    work_data = buffer + sizeof(DictHeader);
}

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

static ExprResult expr1() {
    ExprResult result;
    switch (token.token_type)
    {
    case TOKEN_INTLIT:
        result.data_type = TYPE_INT;
        result.is_literal = true;
        result.int_value = token.int_value;
        NEXT;
        printf("push int %d\n", token.int_value);
        return result;

    case TOKEN_FLOATLIT:
        result.data_type = TYPE_FLOAT;
        result.is_literal = true;
        result.float_value = token.float_value;
        printf("push float %f\n", token.float_value);
        NEXT;
        return result;

    case TOKEN_STRLIT:
        result.data_type = TYPE_STRING;
        result.is_literal = true;
        printf("push str \"%.*s\"\n", (int)token.length, work_data);
        result.pointer = buffer;
        emplace_string(token.length);
        NEXT;
        return result;

    default:
        unexpected();
        break;
    }
}

static ExprResult expr() {
    ExprResult left = expr1();
    while (token.token_type == TOKEN_PLUS)
    {
        expect(TOKEN_PLUS);
        ExprResult next = expr1();
        printf("add\n");
    }
}

void parse(char* _buffer, const char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    expr();
}

