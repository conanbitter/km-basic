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
    "CFLOAT",
    "CINT",
    "CONST",
    "CONTINUE",
    "CSTR",
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

static const struct DecTableRow kw_table[192] = {
    {'A',  1, 9},
    {'N',  2, 0},
    {'D',  3, 0},
    {'\0', TOKEN_KW_AND, 4},
    {'A',  5, 0},
    {'L',  6, 0},
    {'S',  7, 0},
    {'O',  8, 0},
    {'\0', TOKEN_KW_ANDALSO, 0},
    {'C',  10, 39},
    {'A',  11, 14},
    {'S',  12, 0},
    {'E',  13, 0},
    {'\0', TOKEN_KW_CASE, 0},
    {'F',  15, 20},
    {'L',  16, 0},
    {'O',  17, 0},
    {'A',  18, 0},
    {'T',  19, 0},
    {'\0', TOKEN_KW_CFLOAT, 0},
    {'I',  21, 24},
    {'N',  22, 0},
    {'T',  23, 0},
    {'\0', TOKEN_KW_CINT, 0},
    {'O',  25, 35},
    {'N',  26, 0},
    {'S',  27, 29},
    {'T',  28, 0},
    {'\0', TOKEN_KW_CONST, 0},
    {'T',  30, 0},
    {'I',  31, 0},
    {'N',  32, 0},
    {'U',  33, 0},
    {'E',  34, 0},
    {'\0', TOKEN_KW_CONTINUE, 0},
    {'S',  36, 0},
    {'T',  37, 0},
    {'R',  38, 0},
    {'\0', TOKEN_KW_CSTR, 0},
    {'D',  40, 52},
    {'E',  41, 47},
    {'C',  42, 0},
    {'L',  43, 0},
    {'A',  44, 0},
    {'R',  45, 0},
    {'E',  46, 0},
    {'\0', TOKEN_KW_DECLARE, 0},
    {'I',  48, 50},
    {'M',  49, 0},
    {'\0', TOKEN_KW_DIM, 0},
    {'O',  51, 0},
    {'\0', TOKEN_KW_DO, 0},
    {'E',  53, 67},
    {'L',  54, 60},
    {'S',  55, 0},
    {'E',  56, 0},
    {'\0', TOKEN_KW_ELSE, 57},
    {'I',  58, 0},
    {'F',  59, 0},
    {'\0', TOKEN_KW_ELSEIF, 0},
    {'N',  61, 63},
    {'D',  62, 0},
    {'\0', TOKEN_KW_END, 0},
    {'X',  64, 0},
    {'I',  65, 0},
    {'T',  66, 0},
    {'\0', TOKEN_KW_EXIT, 0},
    {'F',  68, 84},
    {'A',  69, 73},
    {'L',  70, 0},
    {'S',  71, 0},
    {'E',  72, 0},
    {'\0', TOKEN_KW_FALSE, 0},
    {'O',  74, 76},
    {'R',  75, 0},
    {'\0', TOKEN_KW_FOR, 0},
    {'U',  77, 0},
    {'N',  78, 0},
    {'C',  79, 0},
    {'T',  80, 0},
    {'I',  81, 0},
    {'O',  82, 0},
    {'N',  83, 0},
    {'\0', TOKEN_KW_FUNCTION, 0},
    {'G',  85, 91},
    {'L',  86, 0},
    {'O',  87, 0},
    {'B',  88, 0},
    {'A',  89, 0},
    {'L',  90, 0},
    {'\0', TOKEN_KW_GLOBAL, 0},
    {'I',  92, 101},
    {'F',  93, 94},
    {'\0', TOKEN_KW_IF, 0},
    {'N',  95, 99},
    {'P',  96, 0},
    {'U',  97, 0},
    {'T',  98, 0},
    {'\0', TOKEN_KW_INPUT, 0},
    {'S',  100, 0},
    {'\0', TOKEN_KW_IS, 0},
    {'L',  102, 109},
    {'E',  103, 105},
    {'N',  104, 0},
    {'\0', TOKEN_KW_LEN, 0},
    {'O',  106, 0},
    {'O',  107, 0},
    {'P',  108, 0},
    {'\0', TOKEN_KW_LOOP, 0},
    {'M',  110, 113},
    {'O',  111, 0},
    {'D',  112, 0},
    {'\0', TOKEN_KW_MOD, 0},
    {'N',  114, 121},
    {'E',  115, 118},
    {'X',  116, 0},
    {'T',  117, 0},
    {'\0', TOKEN_KW_NEXT, 0},
    {'O',  119, 0},
    {'T',  120, 0},
    {'\0', TOKEN_KW_NOT, 0},
    {'O',  122, 129},
    {'R',  123, 0},
    {'\0', TOKEN_KW_OR, 124},
    {'E',  125, 0},
    {'L',  126, 0},
    {'S',  127, 0},
    {'E',  128, 0},
    {'\0', TOKEN_KW_ORELSE, 0},
    {'P',  130, 135},
    {'R',  131, 0},
    {'I',  132, 0},
    {'N',  133, 0},
    {'T',  134, 0},
    {'\0', TOKEN_KW_PRINT, 0},
    {'R',  136, 146},
    {'E',  137, 0},
    {'F',  138, 139},
    {'\0', TOKEN_KW_REF, 0},
    {'M',  140, 141},
    {'\0', TOKEN_KW_REM, 0},
    {'T',  142, 0},
    {'U',  143, 0},
    {'R',  144, 0},
    {'N',  145, 0},
    {'\0', TOKEN_KW_RETURN, 0},
    {'S',  147, 165},
    {'E',  148, 153},
    {'L',  149, 0},
    {'E',  150, 0},
    {'C',  151, 0},
    {'T',  152, 0},
    {'\0', TOKEN_KW_SELECT, 0},
    {'T',  154, 162},
    {'A',  155, 159},
    {'T',  156, 0},
    {'I',  157, 0},
    {'C',  158, 0},
    {'\0', TOKEN_KW_STATIC, 0},
    {'E',  160, 0},
    {'P',  161, 0},
    {'\0', TOKEN_KW_STEP, 0},
    {'U',  163, 0},
    {'B',  164, 0},
    {'\0', TOKEN_KW_SUB, 0},
    {'T',  166, 176},
    {'H',  167, 170},
    {'E',  168, 0},
    {'N',  169, 0},
    {'\0', TOKEN_KW_THEN, 0},
    {'O',  171, 172},
    {'\0', TOKEN_KW_TO, 0},
    {'R',  173, 0},
    {'U',  174, 0},
    {'E',  175, 0},
    {'\0', TOKEN_KW_TRUE, 0},
    {'U',  177, 182},
    {'N',  178, 0},
    {'T',  179, 0},
    {'I',  180, 0},
    {'L',  181, 0},
    {'\0', TOKEN_KW_UNTIL, 0},
    {'W',  183, 188},
    {'H',  184, 0},
    {'I',  185, 0},
    {'L',  186, 0},
    {'E',  187, 0},
    {'\0', TOKEN_KW_WHILE, 0},
    {'X',  189, 0},
    {'O',  190, 0},
    {'R',  191, 0},
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