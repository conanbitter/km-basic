#include <stdio.h>

#include "lexer.h"

int main() {
    open_file("sample.bas");
    while (curchar != '\0') {
        printf("%c", curchar);
        get_char();
    }
    close_file();
    printf("Hello!");
    return 0;
}