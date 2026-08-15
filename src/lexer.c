#include "lexer.h"

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#define INPUT_BUFFER_SIZE (512)

static char input_buffer[INPUT_BUFFER_SIZE];
static char* input_cursor;
static size_t current_size;
static FILE* input_file;
static bool input_eof;

char curchar;

static void refill_buffer() {
    current_size = fread(input_buffer, 1, INPUT_BUFFER_SIZE, input_file);
    input_cursor = input_buffer;
    if (current_size < INPUT_BUFFER_SIZE && feof(input_file)) {
        input_eof = true;
    }
}

void get_char() {
    if (input_cursor >= input_buffer + current_size) {
        if (input_eof) return '\0';
        refill_buffer();
    }
    curchar = *input_cursor;
    input_cursor++;
}

void open_file(const char* filename) {
    input_file = fopen(filename, "r");
    input_eof = false;
    refill_buffer();
    get_char();
}

void close_file() {
    fclose(input_file);
}
