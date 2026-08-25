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
    "ANDTHEN",
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