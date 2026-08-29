#include "common.h"
#include "lexer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define INPUT_BUFFER_SIZE (512)

static char input_buffer[INPUT_BUFFER_SIZE];
static char* input_cursor;
static size_t current_size;
static FILE* input_file;
static bool input_eof;
static int file_line;
static int file_col;

const char* TOKEN_NAMES[] = {
    "ERROR",
    "AND",
    "ANDALSO",
    "CASE",
    "CONST",
    "CONTINUE",
    "DECLARE",
    "DIM",
    "DO",
    "ELSE",
    "ELSEIF",
    "END",
    "EXIT",
    "FALSE",
    "FOR",
    "FUNCTION",
    "GLOBAL",
    "IF",
    "INPUT",
    "IS",
    "LEN",
    "LOOP",
    "MOD",
    "NEXT",
    "NOT",
    "OR",
    "ORELSE",
    "PRINT",
    "REF",
    "REM",
    "RETURN",
    "SELECT",
    "STATIC",
    "STEP",
    "SUB",
    "THEN",
    "TO",
    "TRUE",
    "UNTIL",
    "WHILE",
    "XOR",
    "NEWLINE",
    "EOF",
    "ID",
    "INTLIT",
    "FLOATLIT",
    "STRLIT",
    "(",
    ")",
    ",",
    ";",
    "+",
    "-",
    "*",
    "/",
    "\\",
    "^",
    "&",
    "<<",
    ">>",
    "=",
    "<>",
    ">",
    ">=",
    "<",
    "<="
};

char curchar;

struct DecTableRow {
    char character;
    uint8_t cmd_then;
    uint8_t cmd_else;
};

Token token;

static const struct DecTableRow kw_table[178] = {
    {'A',  1, 9},
    {'N',  2, 0},
    {'D',  3, 0},
    {'\0', TOKEN_KW_AND, 4},
    {'A',  5, 0},
    {'L',  6, 0},
    {'S',  7, 0},
    {'O',  8, 0},
    {'\0', TOKEN_KW_ANDALSO, 0},
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
    {'F',  54, 70},
    {'A',  55, 59},
    {'L',  56, 0},
    {'S',  57, 0},
    {'E',  58, 0},
    {'\0', TOKEN_KW_FALSE, 0},
    {'O',  60, 62},
    {'R',  61, 0},
    {'\0', TOKEN_KW_FOR, 0},
    {'U',  63, 0},
    {'N',  64, 0},
    {'C',  65, 0},
    {'T',  66, 0},
    {'I',  67, 0},
    {'O',  68, 0},
    {'N',  69, 0},
    {'\0', TOKEN_KW_FUNCTION, 0},
    {'G',  71, 77},
    {'L',  72, 0},
    {'O',  73, 0},
    {'B',  74, 0},
    {'A',  75, 0},
    {'L',  76, 0},
    {'\0', TOKEN_KW_GLOBAL, 0},
    {'I',  78, 87},
    {'F',  79, 80},
    {'\0', TOKEN_KW_IF, 0},
    {'N',  81, 85},
    {'P',  82, 0},
    {'U',  83, 0},
    {'T',  84, 0},
    {'\0', TOKEN_KW_INPUT, 0},
    {'S',  86, 0},
    {'\0', TOKEN_KW_IS, 0},
    {'L',  88, 95},
    {'E',  89, 91},
    {'N',  90, 0},
    {'\0', TOKEN_KW_LEN, 0},
    {'O',  92, 0},
    {'O',  93, 0},
    {'P',  94, 0},
    {'\0', TOKEN_KW_LOOP, 0},
    {'M',  96, 99},
    {'O',  97, 0},
    {'D',  98, 0},
    {'\0', TOKEN_KW_MOD, 0},
    {'N',  100, 107},
    {'E',  101, 104},
    {'X',  102, 0},
    {'T',  103, 0},
    {'\0', TOKEN_KW_NEXT, 0},
    {'O',  105, 0},
    {'T',  106, 0},
    {'\0', TOKEN_KW_NOT, 0},
    {'O',  108, 115},
    {'R',  109, 0},
    {'\0', TOKEN_KW_OR, 110},
    {'E',  111, 0},
    {'L',  112, 0},
    {'S',  113, 0},
    {'E',  114, 0},
    {'\0', TOKEN_KW_ORELSE, 0},
    {'P',  116, 121},
    {'R',  117, 0},
    {'I',  118, 0},
    {'N',  119, 0},
    {'T',  120, 0},
    {'\0', TOKEN_KW_PRINT, 0},
    {'R',  122, 132},
    {'E',  123, 0},
    {'F',  124, 125},
    {'\0', TOKEN_KW_REF, 0},
    {'M',  126, 127},
    {'\0', TOKEN_KW_REM, 0},
    {'T',  128, 0},
    {'U',  129, 0},
    {'R',  130, 0},
    {'N',  131, 0},
    {'\0', TOKEN_KW_RETURN, 0},
    {'S',  133, 151},
    {'E',  134, 139},
    {'L',  135, 0},
    {'E',  136, 0},
    {'C',  137, 0},
    {'T',  138, 0},
    {'\0', TOKEN_KW_SELECT, 0},
    {'T',  140, 148},
    {'A',  141, 145},
    {'T',  142, 0},
    {'I',  143, 0},
    {'C',  144, 0},
    {'\0', TOKEN_KW_STATIC, 0},
    {'E',  146, 0},
    {'P',  147, 0},
    {'\0', TOKEN_KW_STEP, 0},
    {'U',  149, 0},
    {'B',  150, 0},
    {'\0', TOKEN_KW_SUB, 0},
    {'T',  152, 162},
    {'H',  153, 156},
    {'E',  154, 0},
    {'N',  155, 0},
    {'\0', TOKEN_KW_THEN, 0},
    {'O',  157, 158},
    {'\0', TOKEN_KW_TO, 0},
    {'R',  159, 0},
    {'U',  160, 0},
    {'E',  161, 0},
    {'\0', TOKEN_KW_TRUE, 0},
    {'U',  163, 168},
    {'N',  164, 0},
    {'T',  165, 0},
    {'I',  166, 0},
    {'L',  167, 0},
    {'\0', TOKEN_KW_UNTIL, 0},
    {'W',  169, 174},
    {'H',  170, 0},
    {'I',  171, 0},
    {'L',  172, 0},
    {'E',  173, 0},
    {'\0', TOKEN_KW_WHILE, 0},
    {'X',  175, 0},
    {'O',  176, 0},
    {'R',  177, 0},
    {'\0', TOKEN_KW_XOR, 0},
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
        if (input_eof) {
            curchar = '\0';
            return;
        }
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

bool is_space(char symbol) {
    return curchar == ' ' || curchar == '\t';
}

void skip_spaces() {
    while (is_space(curchar)) next_char();
}

void skip_newlines() {
    while (curchar == '\n' || curchar == '\r' || curchar == ':') {
        if (curchar == '\n') {
            file_line++;
            file_col = 0;
        }
        next_char();
    }
}

void skip_comment() {
    while (curchar != '\r' && curchar != '\n' && curchar != '\0')
    {
        next_char();
    }
}

static void begin_token() {
    token.line = file_line;
    token.col = file_col;
}

static void set_token(TokenType token_type) {
    token.token_type = token_type;
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

static bool is_alpha(char symbol) {
    return (symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') || symbol == '_';
}

static bool is_numeric(char symbol) {
    return symbol >= '0' && symbol <= '9';
}

static bool is_alphanum(char symbol) {
    return is_alpha(symbol) || is_numeric(symbol);
}

static void copy_char(char** buffer, const char* buffer_end) {
    if (*buffer >= buffer_end) {
        printf("Run out of memory\n");
        exit(1);
    }
    **buffer = curchar;
    (*buffer)++;
    next_char();
}

static void add_char(char symbol, char** buffer, const char* buffer_end) {
    if (*buffer >= buffer_end) {
        printf("Run out of memory\n");
        exit(1);
    }
    **buffer = symbol;
    (*buffer)++;
}

static void process_int(char** buffer, const char* buffer_end) {
    while (is_numeric(curchar))
    {
        copy_char(buffer, buffer_end);
    }
}

static void process_float(char** buffer, const char* buffer_end) {
    if (curchar == '.') {
        copy_char(buffer, buffer_end); // copy '.'
        process_int(buffer, buffer_end);
    }
    if (curchar == 'e' || curchar == 'E') {
        copy_char(buffer, buffer_end); // copy 'e'
        if (curchar == '+' || curchar == '-') {
            copy_char(buffer, buffer_end); // copy sign
        }
        process_int(buffer, buffer_end);
    }
}

void next_token(char* buffer, const char* buffer_end) {
    while (true) {
        bool was_spaces = is_space(curchar);
        skip_spaces();
        if (curchar == '\'') {
            skip_comment();
            break;
        }

        begin_token();

        switch (curchar)
        {
        case '\0': set_token(TOKEN_EOF); return;
        case '\r':
        case '\n':
        case ':':
            skip_newlines();
            if (token.token_type == TOKEN_NEWLINE) {
                break;
            } else {
                set_token(TOKEN_NEWLINE);
                return;
            }
        case '(': set_token_next(TOKEN_LPAREN); return;
        case ')': set_token_next(TOKEN_RPAREN); return;
        case ',': set_token_next(TOKEN_COMMA); return;
        case ';': set_token_next(TOKEN_SEMICOLON); return;
        case '+': set_token_next(TOKEN_PLUS); return;
        case '-': set_token_next(TOKEN_MINUS); return;
        case '*': set_token_next(TOKEN_MUL); return;
        case '/': set_token_next(TOKEN_DIV); return;
        case '\\': set_token_next(TOKEN_INTDIV); return;
        case '^': set_token_next(TOKEN_POWER); return;
        case '&': set_token_next(TOKEN_CONCAT); return;
        case '=': set_token_next(TOKEN_EQ); return;
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
            return;

        case '>':
            next_char();
            if (curchar == '>') {
                set_token_next(TOKEN_RSHIFT);
            } else if (curchar == '=') {
                set_token_next(TOKEN_GTEQ);
            } else {
                set_token(TOKEN_GT);
            }
            return;

        case '.': {
            char* end;
            const char* start = buffer;
            process_float(&buffer, buffer_end);
            add_char('\0', &buffer, buffer_end);
            set_token_float(TOKEN_FLOATLIT, strtof(start, &end));
            return;
        }

        case '"': {
            next_char();
            size_t length = 0;
            while (curchar != '\0')
            {
                if (curchar == '"') {
                    next_char();
                    if (curchar == '"') {
                        add_char('"', &buffer, buffer_end);
                        length++;
                        next_char();
                    } else {
                        set_token_len(TOKEN_STRLIT, length);
                        break;
                    }
                } else {
                    copy_char(&buffer, buffer_end);
                    length++;
                }
            }
            return;
        }

        default:
            if (is_alpha(curchar)) {
                const char* start = buffer;
                size_t length = 0;
                while (is_alphanum(curchar))
                {
                    if (curchar >= 'a' && curchar <= 'z') {
                        add_char(curchar - 'a' + 'A', &buffer, buffer_end);
                        next_char();
                    } else {
                        copy_char(&buffer, buffer_end);
                    }
                    length++;
                }
                if (curchar == '#' || curchar == '$') {
                    copy_char(&buffer, buffer_end);
                    length++;
                }
                // newline skipper
                if (length == 1 && *start == '_' && was_spaces) {
                    if (curchar == '\r' || curchar == '\n') {
                        skip_newlines();
                        break;
                    }
                }

                add_char('\0', &buffer, buffer_end);
                TokenType kw = check_keyword(start);
                if (kw == TOKEN_KW_REM) {
                    skip_comment();
                    break;
                } else if (kw != TOKEN_ERROR) {
                    set_token(kw);
                } else {
                    set_token_len(TOKEN_ID, length);
                }
            } else if (is_numeric(curchar)) {
                char* end;
                const char* start = buffer;
                process_int(&buffer, buffer_end);
                if (curchar == '.' || curchar == 'e' || curchar == 'E') {
                    process_float(&buffer, buffer_end);
                    add_char('\0', &buffer, buffer_end);
                    set_token_float(TOKEN_FLOATLIT, strtof(start, &end));
                } else {
                    add_char('\0', &buffer, buffer_end);
                    set_token_int(TOKEN_INTLIT, strtol(start, &end, 10));
                }
            } else {
                set_token(TOKEN_ERROR);
                printf("[%d:%d] ERROR: unknown symbol '%c'\n", file_line, file_col, curchar);
                exit(1);
            }
            return;
        }
    }
}

void print_token(char* buffer) {
    printf("[%d:%d] %s", token.line, token.col, TOKEN_NAMES[token.token_type]);
    switch (token.token_type) {
    case TOKEN_INTLIT:
        printf(" = %" PRIkmINT, token.int_value);
        break;
    case TOKEN_FLOATLIT:
        printf(" = %f", token.float_value);
        break;
    case TOKEN_ID:
        printf(" '%.*s'", (int)token.length, buffer);
        break;
    case TOKEN_STRLIT:
        printf(" \"%.*s\"", (int)token.length, buffer);
        break;
    }
}