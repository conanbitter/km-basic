#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"

#include <stdbool.h>
#include <stdalign.h>

typedef struct DictHeader {
    struct DictHeader* prev;
    uint32_t hash;
} DictHeader;

typedef struct ExprResult {
    DataType data_type;
    bool is_literal;
    union {
        KmInt int_value;
        KmFloat float_value;
    };
    TreeNode* node;
} ExprResult;

static char* buffer;
static char* buffer_end;
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
    return hash;
}

static void emplace_string(size_t length) {
    DictHeader* current = (DictHeader*)buffer;
    current->prev = prev;
    current->hash = get_hash(work_data, length, true);
    prev = current;
    buffer = align_ptr(buffer + sizeof(DictHeader) + length);
    work_data = buffer + sizeof(DictHeader);
}

static TreeNode* add_node() {
    if ((buffer_end - sizeof(TreeNode)) <= buffer) {
        printf("Run out of memory");
        exit(1);
    }
    buffer_end -= sizeof(TreeNode);
    return (TreeNode*)buffer_end;
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

static ExprResult conv2float(ExprResult intnode) {
    ExprResult result;
    result.data_type = TYPE_FLOAT;
    result.is_literal = intnode.is_literal;
    if (intnode.is_literal) {
        result.float_value = intnode.int_value;
    } else {
        TreeNode* node = add_node();
        node->node_type = NODE_ITOF;
        node->child = intnode.node;

        result.node = node;
    }
    return result;
}

static ExprResult expr1() {
    ExprResult result;
    TreeNode* node;
    switch (token.token_type)
    {
    case TOKEN_INTLIT:
        node = add_node();
        node->node_type = NODE_INTLIT;
        node->intlit = token.int_value;

        result.data_type = TYPE_INT;
        result.is_literal = false;
        result.int_value = token.int_value;
        result.node = node;
        NEXT;
        return result;

    case TOKEN_FLOATLIT:
        node = add_node();
        node->node_type = NODE_FLOATLIT;
        node->floatlit = token.float_value;

        result.data_type = TYPE_FLOAT;
        result.is_literal = false;
        result.float_value = token.float_value;
        result.node = node;
        NEXT;
        return result;

    case TOKEN_STRLIT:
        result.data_type = TYPE_STRING;
        result.is_literal = true;
        //result.pointer = buffer;
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
        TokenType tt = token.token_type;
        int line = token.line;
        int col = token.col;
        if (left.data_type != TYPE_INT && left.data_type != TYPE_FLOAT) {
            printf("[%d:%d] ERROR: wrong type for left operand of '%c'. Must be INTEGER or FLOAT\n", line, col, tt == TOKEN_PLUS ? '+' : '-');
            exit(1);
        }
        expect(TOKEN_PLUS);
        ExprResult next = expr1();
        if (next.data_type != TYPE_INT && next.data_type != TYPE_FLOAT) {
            printf("[%d:%d] ERROR: wrong type for right operand of '%c'. Must be INTEGER or FLOAT\n", line, col, tt == TOKEN_PLUS ? '+' : '-');
            exit(1);
        }

        if (left.data_type == TYPE_INT && next.data_type == TYPE_FLOAT) {
            left = conv2float(left);
        }
        if (left.data_type == TYPE_FLOAT && next.data_type == TYPE_INT) {
            next = conv2float(next);
        }

        TreeNode* node = add_node();
        node->node_type = NODE_BINOP;
        node->binop.op = left.data_type == TYPE_INT ? BINOP_IADD : BINOP_FADD;
        node->binop.left = left.node;
        node->binop.right = next.node;

        left.is_literal = false;
        left.node = node;
    }
    return left;
}

void debug_print_tree(char* start, char* end) {
    TreeNode* _end = (TreeNode*)end;
    TreeNode* cur = (TreeNode*)start;
    while (cur != _end)
    {
        printf("%p ", (void*)cur);
        switch (cur->node_type)
        {
        case NODE_BINOP:
            printf("binop     %d  %p, %p\n", cur->binop.op, (void*)(cur->binop.left), (void*)(cur->binop.right));
            break;

        case NODE_FLOATLIT:
            printf("floatlit  %f\n", cur->floatlit);
            break;

        case NODE_INTLIT:
            printf("intlit    %d\n", cur->intlit);
            break;

        case NODE_ITOF:
            printf("itof      %p\n", (void*)(cur->child));
            break;
        }
        cur++;
    }

}

void parse(char* _buffer, char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    ExprResult res = expr();
    printf("Root = %p\n", (void*)res.node);
    debug_print_tree(buffer_end, _buffer_end);
}

