#ifndef LEXER_H
#define LEXER_H

#include "common.h"

typedef enum TokenType {
    TOKEN_ERROR = 0,

    TOKEN_KW_AND,
    TOKEN_KW_ANDTHEN,
    TOKEN_KW_CASE,
    TOKEN_KW_CONST,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_DECLARE,
    TOKEN_KW_DIM,
    TOKEN_KW_DO,
    TOKEN_KW_ELSE,
    TOKEN_KW_ELSEIF,
    TOKEN_KW_END,
    TOKEN_KW_EXIT,
    TOKEN_KW_FOR,
    TOKEN_KW_FUNCTION,
    TOKEN_KW_GLOBAL,
    TOKEN_KW_IF,
    TOKEN_KW_INPUT,
    TOKEN_KW_IS,
    TOKEN_KW_LEN,
    TOKEN_KW_LOOP,
    TOKEN_KW_MOD,
    TOKEN_KW_NEXT,
    TOKEN_KW_NOT,
    TOKEN_KW_OR,
    TOKEN_KW_ORELSE,
    TOKEN_KW_PRINT,
    TOKEN_KW_REF,
    TOKEN_KW_REM,
    TOKEN_KW_RETURN,
    TOKEN_KW_SELECT,
    TOKEN_KW_STATIC,
    TOKEN_KW_STEP,
    TOKEN_KW_SUB,
    TOKEN_KW_THEN,
    TOKEN_KW_TO,
    TOKEN_KW_UNTIL,
    TOKEN_KW_WHILE,
    TOKEN_KW_XOR,

    TOKEN_NEWLINE,
    TOKEN_EOF,

    TOKEN_ID,
    TOKEN_INTLIT,
    TOKEN_FLOATLIT,
    TOKEN_STRLIT,

    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_INTDIV,
    TOKEN_POWER,
    TOKEN_CONCAT,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_GT,
    TOKEN_GTEQ,
    TOKEN_LS,
    TOKEN_LSEQ
} TokenType;

typedef struct Token {

    TokenType token_type;

    int line;
    int col;

    union {
        KmInt int_value;
        KmFloat float_value;
        size_t length;
    };

} Token;

void open_file(const char* filename);
void close_file();
void next_token(char* buffer, size_t buffer_length);
void print_token(char* buffer);

extern Token token;

#endif