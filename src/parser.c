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
        TreeNode* node;
    };
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
        node->node_type = NODE_EXPROP;
        node->exprop.op = UNOP_ITOF;
        node->exprop.left = intnode.node;
        node->exprop.right = NULL;

        result.node = node;
    }
    return result;
}

static ExprResult expr13() {
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
        //result.pointer = buffer;
        emplace_string(token.length);
        NEXT;
        return result;

    case TOKEN_ID:
        char suffix = *(work_data + token.length - 1);
        result.data_type = suffix == '#' ? TYPE_FLOAT : TYPE_INT;
        result.is_literal = false;
        result.node = add_node();
        result.node->node_type = NODE_LOAD;
        result.node->load.is_local = false;
        result.node->load.offset = 0;
        NEXT;
        return result;

    default:
        unexpected();
        break;
    }
}

static ExprResult make_node(ExprResult res) {
    ExprResult noderes;
    TreeNode* node = add_node();
    noderes.is_literal = false;
    noderes.data_type = res.data_type;
    noderes.node = node;
    switch (res.data_type)
    {
    case TYPE_INT:
        node->node_type = NODE_INTLIT;
        node->intlit = res.int_value;
        break;
    case TYPE_FLOAT:
        node->node_type = NODE_FLOATLIT;
        node->floatlit = res.float_value;
        break;
    }
    return noderes;
}

static ExprResult expr7() {
    ExprResult left = expr13();
    while (token.token_type == TOKEN_PLUS || token.token_type == TOKEN_MINUS)
    {
        TokenType tt = token.token_type;
        int line = token.line;
        int col = token.col;
        if (left.data_type != TYPE_INT && left.data_type != TYPE_FLOAT) {
            printf("[%d:%d] ERROR: wrong type for left operand of '%c'. Must be INTEGER or FLOAT\n", line, col, tt == TOKEN_PLUS ? '+' : '-');
            exit(1);
        }
        NEXT;
        ExprResult next = expr13();
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

        if (left.is_literal && next.is_literal) {
            if (left.data_type == TYPE_INT) {
                if (tt == TOKEN_PLUS) {
                    left.int_value += next.int_value;
                } else {
                    left.int_value -= next.int_value;
                }
            }
            if (left.data_type == TYPE_FLOAT) {
                if (tt == TOKEN_PLUS) {
                    left.float_value += next.float_value;
                } else {
                    left.float_value -= next.float_value;
                }
            }
        } else {
            if (left.is_literal) left = make_node(left);
            if (next.is_literal) next = make_node(next);

            TreeNode* node = add_node();
            node->node_type = NODE_EXPROP;
            node->exprop.op = left.data_type == TYPE_INT ? (tt == TOKEN_PLUS ? BINOP_IADD : BINOP_ISUB) : (tt == TOKEN_PLUS ? BINOP_FADD : BINOP_FSUB);
            node->exprop.left = left.node;
            node->exprop.right = next.node;

            left.is_literal = false;
            left.node = node;
        }
    }
    return left;
}

void parse(char* _buffer, char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    ExprResult res = expr7();
    if (res.is_literal) res = make_node(res);
    printf("Root = %d\n", (uintptr_t)res.node - (uintptr_t)buffer_end);
    debug_print_tree(buffer_end, _buffer_end);
}

