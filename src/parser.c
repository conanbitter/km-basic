#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"

#include <stdbool.h>
#include <stdalign.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

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

#pragma region Dictionary

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

#pragma endregion

#pragma region Parser service functions

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

#pragma endregion

#pragma region Expressions

// SERVICE FUNCTIONS

static void int2float(ExprResult* res) {
    res->data_type = TYPE_FLOAT;
    if (res->is_literal) {
        res->float_value = res->int_value;
    } else {
        TreeNode* node = add_node();
        node->node_type = NODE_EXPROP;
        node->exprop.op = UNOP_ITOF;
        node->exprop.left = res->node;
        node->exprop.right = NULL;

        res->node = node;
    }
}

static void float2int(ExprResult* res) {
    res->data_type = TYPE_INT;
    if (res->is_literal) {
        res->int_value = res->float_value;
    } else {
        TreeNode* node = add_node();
        node->node_type = NODE_EXPROP;
        node->exprop.op = UNOP_FTOI;
        node->exprop.left = res->node;
        node->exprop.right = NULL;

        res->node = node;
    }
}

static void type_cast(ExprResult* res, DataType target_type) {
    if (res->data_type == TYPE_INT && target_type == TYPE_FLOAT) {
        int2float(res);
    }
    if (res->data_type == TYPE_FLOAT && target_type == TYPE_INT) {
        float2int(res);
    }
}

static TreeNode* as_node(ExprResult res) {
    if (!res.is_literal) return res.node;

    TreeNode* node = add_node();
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

    return node;
}

const char* type2str(DataType datatype) {
    switch (datatype) {
    case TYPE_INT: return "INTEGER";
    case TYPE_FLOAT: return "FLOAT";
    case TYPE_STRING: return "STRING";
    case TYPE_INT | TYPE_FLOAT: return "INTEGER or FLOAT";
    default: return "UNKNOWN";
    }
}

static void check_and_cast(ExprResult* left, ExprResult* right, DataType in_types, DataType out_types, Token* optoken) {
    // type check
    if ((left->data_type & in_types) == 0) {
        printf("[%d:%d] ERROR: wrong type for left operand of '%s'. Type is %s, must be %s.",
            optoken->line,
            optoken->col,
            TOKEN_NAMES[optoken->token_type],
            type2str(left->data_type),
            type2str(in_types)
        );
        exit(1);
    }

    if ((right->data_type & in_types) == 0) {
        printf("[%d:%d] ERROR: wrong type for right operand of '%s'. Type is %s, must be %s.",
            optoken->line,
            optoken->col,
            TOKEN_NAMES[optoken->token_type],
            type2str(right->data_type),
            type2str(in_types)
        );
        exit(1);
    }

    // type cast
    DataType res_type = out_types == (TYPE_INT | TYPE_FLOAT) ? MAX(left->data_type, right->data_type) : out_types;
    type_cast(left, res_type);
    type_cast(right, res_type);
}

static ExprResult binop_expr(ExprResult left, ExprResult right, ExprOpType optype) {
    TreeNode* node = add_node();
    node->node_type = NODE_EXPROP;
    node->exprop.op = optype;
    node->exprop.left = as_node(left);
    node->exprop.right = as_node(right);

    left.is_literal = false;
    left.node = node;

    return left;
}

// EXPRESSIONS

static ExprResult expr();

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

    case TOKEN_LPAREN:
        expect(TOKEN_LPAREN);
        result = expr();
        expect(TOKEN_RPAREN);
        return result;

    default:
        unexpected();
        break;
    }
}

// ^ (power)
static ExprResult expr12() {
    return expr13();
}

// + - (unary)
static ExprResult expr11() {
    return expr12();
}

// * /
static ExprResult expr10() {
    ExprResult left = expr11();

    while (token.token_type == TOKEN_MUL || token.token_type == TOKEN_DIV)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr11();

        DataType out_types = token.token_type == TOKEN_MUL ? TYPE_INT | TYPE_FLOAT : TYPE_FLOAT;
        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, out_types, &optoken);

        if (left.is_literal && right.is_literal) {
            if (token.token_type == TOKEN_MUL) {
                if (left.data_type == TYPE_INT) {
                    left.int_value *= right.int_value;
                } else {
                    left.float_value *= right.float_value;
                }
            } else {
                left.float_value /= right.float_value;
            }
        } else {
            DataType op_type = optoken.token_type == TOKEN_MUL ?
                (left.data_type == TYPE_INT ? BINOP_IMUL : BINOP_FMUL) : BINOP_FDIV;
            left = binop_expr(left, right, op_type);
        }
    }
    return left;
}

// \ (intdiv)
static ExprResult expr9() {
    ExprResult left = expr10();

    while (token.token_type == TOKEN_INTDIV)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr10();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT, &optoken);

        if (left.is_literal && right.is_literal) {
            left.int_value /= right.int_value;
        } else {
            left = binop_expr(left, right, BINOP_IDIV);
        }
    }
    return left;
}

// MOD
static ExprResult expr8() {
    return expr9();
}

// + -
static ExprResult expr7() {
    ExprResult left = expr8();

    while (token.token_type == TOKEN_PLUS || token.token_type == TOKEN_MINUS)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr8();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT | TYPE_FLOAT, &token);

        if (left.is_literal && right.is_literal) {
            if (left.data_type == TYPE_INT) {
                if (optoken.token_type == TOKEN_PLUS) {
                    left.int_value += right.int_value;
                } else {
                    left.int_value -= right.int_value;
                }
            }
            if (left.data_type == TYPE_FLOAT) {
                if (optoken.token_type == TOKEN_PLUS) {
                    left.float_value += right.float_value;
                } else {
                    left.float_value -= right.float_value;
                }
            }
        } else {
            DataType op_type = left.data_type == TYPE_INT ?
                (optoken.token_type == TOKEN_PLUS ? BINOP_IADD : BINOP_ISUB) :
                (optoken.token_type == TOKEN_PLUS ? BINOP_FADD : BINOP_FSUB);

            left = binop_expr(left, right, op_type);
        }
    }
    return left;
}

// &
static ExprResult expr6() {
    return expr7();
}

// << >>
static ExprResult expr5() {
    return expr6();
}

// = <> > >= < <=
static ExprResult expr4() {
    return expr5();
}

// NOT
static ExprResult expr3() {
    return expr4();
}

// AND ANDALSO
static ExprResult expr2() {
    return expr3();
}

// OR ORELSE
static ExprResult expr1() {
    return expr2();
}

// XOR
static ExprResult expr() {
    return expr1();
}

#pragma endregion

void parse(char* _buffer, char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    ExprResult res = expr();
    TreeNode* res_node = as_node(res);
    printf("Root = %d\n", (uintptr_t)res_node - (uintptr_t)buffer_end);
    debug_print_tree(buffer_end, _buffer_end);
}

