#include "common.h"
#include "lexer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define INPUT_BUFFER_SIZE (512)

static char input_buffer[INPUT_BUFFER_SIZE];
static char* input_cursor;
static size_t current_size;
static FILE* input_file;
static bool input_eof;
static int file_line;
static int file_col;

char curchar;

struct DecTableRow {
    char character;
    uint8_t cmd_then;
    uint8_t cmd_else;
};

Token token;

static char temp[256];

static const struct DecTableRow kw_table[169] = {
    {'A',  1, 9},
    {'N',  2, 0},
    {'D',  3, 0},
    {'\0', TOKEN_KW_AND, 4},
    {'T',  5, 0},
    {'H',  6, 0},
    {'E',  7, 0},
    {'N',  8, 0},
    {'\0', TOKEN_KW_ANDTHEN, 0},
    {'C',  10, 25},
    {'A',  11, 14},
    {'S',  12, 0},
    {'E',  13, 0},
    {'\0', TOKEN_KW_CASE, 0},
    {'O',  15, 0},
    {'N',  16, 0},
    {'S',  17, 19},
    {'T',  18, 0},
    {'\0', TOKEN_KW_CONST, 0},
    {'T',  20, 0},
    {'I',  21, 0},
    {'N',  22, 0},
    {'U',  23, 0},
    {'E',  24, 0},
    {'\0', TOKEN_KW_CONTINUE, 0},
    {'D',  26, 38},
    {'E',  27, 33},
    {'C',  28, 0},
    {'L',  29, 0},
    {'A',  30, 0},
    {'R',  31, 0},
    {'E',  32, 0},
    {'\0', TOKEN_KW_DECLARE, 0},
    {'I',  34, 36},
    {'M',  35, 0},
    {'\0', TOKEN_KW_DIM, 0},
    {'O',  37, 0},
    {'\0', TOKEN_KW_DO, 0},
    {'E',  39, 53},
    {'L',  40, 46},
    {'S',  41, 0},
    {'E',  42, 0},
    {'\0', TOKEN_KW_ELSE, 43},
    {'I',  44, 0},
    {'F',  45, 0},
    {'\0', TOKEN_KW_ELSEIF, 0},
    {'N',  47, 49},
    {'D',  48, 0},
    {'\0', TOKEN_KW_END, 0},
    {'X',  50, 0},
    {'I',  51, 0},
    {'T',  52, 0},
    {'\0', TOKEN_KW_EXIT, 0},
    {'F',  54, 65},
    {'O',  55, 57},
    {'R',  56, 0},
    {'\0', TOKEN_KW_FOR, 0},
    {'U',  58, 0},
    {'N',  59, 0},
    {'C',  60, 0},
    {'T',  61, 0},
    {'I',  62, 0},
    {'O',  63, 0},
    {'N',  64, 0},
    {'\0', TOKEN_KW_FUNCTION, 0},
    {'G',  66, 72},
    {'L',  67, 0},
    {'O',  68, 0},
    {'B',  69, 0},
    {'A',  70, 0},
    {'L',  71, 0},
    {'\0', TOKEN_KW_GLOBAL, 0},
    {'I',  73, 82},
    {'F',  74, 75},
    {'\0', TOKEN_KW_IF, 0},
    {'N',  76, 80},
    {'P',  77, 0},
    {'U',  78, 0},
    {'T',  79, 0},
    {'\0', TOKEN_KW_INPUT, 0},
    {'S',  81, 0},
    {'\0', TOKEN_KW_IS, 0},
    {'L',  83, 90},
    {'E',  84, 86},
    {'N',  85, 0},
    {'\0', TOKEN_KW_LEN, 0},
    {'O',  87, 0},
    {'O',  88, 0},
    {'P',  89, 0},
    {'\0', TOKEN_KW_LOOP, 0},
    {'M',  91, 94},
    {'O',  92, 0},
    {'D',  93, 0},
    {'\0', TOKEN_KW_MOD, 0},
    {'N',  95, 102},
    {'E',  96, 99},
    {'X',  97, 0},
    {'T',  98, 0},
    {'\0', TOKEN_KW_NEXT, 0},
    {'O',  100, 0},
    {'T',  101, 0},
    {'\0', TOKEN_KW_NOT, 0},
    {'O',  103, 110},
    {'R',  104, 0},
    {'\0', TOKEN_KW_OR, 105},
    {'E',  106, 0},
    {'L',  107, 0},
    {'S',  108, 0},
    {'E',  109, 0},
    {'\0', TOKEN_KW_ORELSE, 0},
    {'P',  111, 116},
    {'R',  112, 0},
    {'I',  113, 0},
    {'N',  114, 0},
    {'T',  115, 0},
    {'\0', TOKEN_KW_PRINT, 0},
    {'R',  117, 127},
    {'E',  118, 0},
    {'F',  119, 120},
    {'\0', TOKEN_KW_REF, 0},
    {'M',  121, 122},
    {'\0', TOKEN_KW_REM, 0},
    {'T',  123, 0},
    {'U',  124, 0},
    {'R',  125, 0},
    {'N',  126, 0},
    {'\0', TOKEN_KW_RETURN, 0},
    {'S',  128, 146},
    {'E',  129, 134},
    {'L',  130, 0},
    {'E',  131, 0},
    {'C',  132, 0},
    {'T',  133, 0},
    {'\0', TOKEN_KW_SELECT, 0},
    {'T',  135, 143},
    {'A',  136, 140},
    {'T',  137, 0},
    {'I',  138, 0},
    {'C',  139, 0},
    {'\0', TOKEN_KW_STATIC, 0},
    {'E',  141, 0},
    {'P',  142, 0},
    {'\0', TOKEN_KW_STEP, 0},
    {'U',  144, 0},
    {'B',  145, 0},
    {'\0', TOKEN_KW_SUB, 0},
    {'T',  147, 153},
    {'H',  148, 151},
    {'E',  149, 0},
    {'N',  150, 0},
    {'\0', TOKEN_KW_THEN, 0},
    {'O',  152, 0},
    {'\0', TOKEN_KW_TO, 0},
    {'U',  154, 159},
    {'N',  155, 0},
    {'T',  156, 0},
    {'I',  157, 0},
    {'L',  158, 0},
    {'\0', TOKEN_KW_UNTIL, 0},
    {'W',  160, 165},
    {'H',  161, 0},
    {'I',  162, 0},
    {'L',  163, 0},
    {'E',  164, 0},
    {'\0', TOKEN_KW_WHILE, 0},
    {'X',  166, 0},
    {'O',  167, 0},
    {'R',  168, 0},
    {'\0', TOKEN_KW_XOR, 0}
};

static void refill_buffer() {
    current_size = fread(input_buffer, 1, INPUT_BUFFER_SIZE, input_file);
    input_cursor = input_buffer;
    if (current_size < INPUT_BUFFER_SIZE && feof(input_file)) {
        input_eof = true;
    }
}

static void next_char() {
    if (input_cursor >= input_buffer + current_size) {
        if (input_eof) return '\0';
        refill_buffer();
    }
    curchar = *input_cursor;
    input_cursor++;
    file_col++;
}

void open_file(const char* filename) {
    input_file = fopen(filename, "r");
    input_eof = false;
    file_line = 1;
    file_col = 0;
    refill_buffer();
    next_char();
}

void close_file() {
    fclose(input_file);
}

TokenType check_keyword(const char* name) {
    int row = 0;
    const char* cur = name;
    while (1) {
        if (kw_table[row].character == *cur) {
            if (*cur == '\0') {
                return kw_table[row].cmd_then;
            }
            cur++;
            row = kw_table[row].cmd_then;
        } else {
            if (kw_table[row].cmd_else == 0) {
                return TOKEN_ERROR;
            }
            row = kw_table[row].cmd_else;
        }
    }
}

void skip_spaces() {
    while (curchar == ' ' || curchar == '\t') next_char();
}

void skip_newlines() {
    while (curchar == '\n' || curchar == '\r' || curchar == ':') {
        if (curchar == '\n') {
            file_line++;
            file_col = 1;
        }
        next_char();
    }
}

void skip_comment() {
    while (curchar != '\r' || curchar != '\n' || curchar != '\0')
    {
        next_char();
    }
}

static void set_token(TokenType token_type) {
    token.token_type = token_type;
    token.line = file_line;
    token.col = file_col;
}

static void set_token_next(TokenType token_type) {
    set_token(token_type);
    next_char();
}

static void set_token_int(TokenType token_type, KmInt value) {
    set_token(token_type);
    token.int_value = value;
}

static void set_token_float(TokenType token_type, KmFloat value) {
    set_token(token_type);
    token.float_value = value;
}

static void set_token_len(TokenType token_type, size_t value) {
    set_token(token_type);
    token.length = value;
}

void next_token(char* buffer, size_t buffer_length) {
    skip_spaces();
    if (curchar == '\'') {
        skip_comment();
    }

    switch (curchar)
    {
    case '\0': set_token(TOKEN_EOF); break;
    case '\r':
    case '\n':
    case ':':
        skip_newlines();
        set_token(TOKEN_NEWLINE);
        break;
    case '(': set_token_next(TOKEN_LPAREN); break;
    case ')': set_token_next(TOKEN_RPAREN); break;
    case ',': set_token_next(TOKEN_COMMA); break;
    case ';': set_token_next(TOKEN_SEMICOLON); break;
    case '+': set_token_next(TOKEN_PLUS); break;
    case '-': set_token_next(TOKEN_MINUS); break;
    case '*': set_token_next(TOKEN_MUL); break;
    case '/': set_token_next(TOKEN_DIV); break;
    case '\\': set_token_next(TOKEN_INTDIV); break;
    case '^': set_token_next(TOKEN_POWER); break;
    case '&': set_token_next(TOKEN_CONCAT); break;
    case '=': set_token_next(TOKEN_EQ); break;
    case '<':
        next_char();
        if (curchar == '<') {
            set_token_next(TOKEN_LSHIFT);
        } else if (curchar == '=') {
            set_token_next(TOKEN_LSEQ);
        } else if (curchar == '>') {
            set_token_next(TOKEN_NEQ);
        } else {
            set_token(TOKEN_LS);
        }
        break;

    case '>':
        next_char();
        if (curchar == '>') {
            set_token_next(TOKEN_RSHIFT);
        } else if (curchar == '=') {
            set_token_next(TOKEN_GTEQ);
        } else {
            set_token(TOKEN_GT);
        }
        break;

    default:
        break;
    }
}