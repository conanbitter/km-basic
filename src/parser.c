#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"

#include <stdbool.h>
#include <stdalign.h>
#include <math.h>

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
    switch (target_type)
    {
    case TYPE_INT:
        switch (res->data_type)
        {
        case TYPE_FLOAT:
            float2int(res);
            break;
        case TYPE_STRING:
            res->data_type = TYPE_STRING;
            res->is_literal = false;
            res->node = add_node();
            res->node->node_type = NODE_DUMMY;
            break;
        }
        break;

    case TYPE_FLOAT:
        switch (res->data_type)
        {
        case TYPE_INT:
            int2float(res);
            break;
        case TYPE_STRING:
            res->data_type = TYPE_STRING;
            res->is_literal = false;
            res->node = add_node();
            res->node->node_type = NODE_DUMMY;
            break;
        }
        break;

    case TYPE_STRING:
        switch (res->data_type)
        {
        case TYPE_INT:
            res->data_type = TYPE_INT;
            res->is_literal = false;
            res->node = add_node();
            res->node->node_type = NODE_DUMMY;
            break;
        case TYPE_FLOAT:
            res->data_type = TYPE_FLOAT;
            res->is_literal = false;
            res->node = add_node();
            res->node->node_type = NODE_DUMMY;
            break;
        }
        break;
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
    case TYPE_STRING:
        node->node_type = NODE_STRLIT;
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

static void check_unary(ExprResult* operand, DataType in_types, Token* optoken) {
    // type check
    if ((operand->data_type & in_types) == 0) {
        printf("[%d:%d] ERROR: wrong type for operand of unary '%s'. Type is %s, must be %s.",
            optoken->line,
            optoken->col,
            TOKEN_NAMES[optoken->token_type],
            type2str(operand->data_type),
            type2str(in_types)
        );
        exit(1);
    }
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

static ExprResult unop_expr(ExprResult operand, ExprOpType optype) {
    TreeNode* node = add_node();
    node->node_type = NODE_EXPROP;
    node->exprop.op = optype;
    node->exprop.left = as_node(operand);
    node->exprop.right = NULL;

    operand.is_literal = false;
    operand.node = node;

    return operand;
}

// EXPRESSIONS

static ExprResult expr0();

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
        result = expr0();
        expect(TOKEN_RPAREN);
        return result;

    case TOKEN_KW_TRUE:
        result.data_type = TYPE_INT;
        result.is_literal = true;
        result.int_value = KM_TRUE;
        NEXT;
        return result;

    case TOKEN_KW_FALSE:
        result.data_type = TYPE_INT;
        result.is_literal = true;
        result.int_value = KM_FALSE;
        NEXT;
        return result;

    default:
        unexpected();
        break;
    }
}

// ^ (power)
static ExprResult expr12() {
    ExprResult left = expr13();

    while (token.token_type == TOKEN_POWER)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr13();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT | TYPE_FLOAT, &optoken);

        if (right.is_literal && right.data_type == TYPE_INT && right.int_value < 0) {
            printf("[%d:%d] ERROR: Attempt to exponetiate integer with negative exponent (%" PRIkmINT ").",
                optoken.line,
                optoken.col,
                right.int_value
            );
            exit(1);
        }

        if (left.is_literal && right.is_literal) {
            if (left.data_type == TYPE_INT) {
                left.int_value = ipow(left.int_value, right.int_value);
            } else {
                left.float_value = powf(left.float_value, right.float_value);
            }
        } else {
            left = binop_expr(left, right, left.data_type == TYPE_INT ? BINOP_IPOWER : BINOP_FPOWER);
        }
    }
    return left;
}

// + - (unary)
static ExprResult expr11() {
    switch (token.token_type)
    {
    case TOKEN_PLUS:
        NEXT;
        return expr12();
        break;

    case TOKEN_MINUS: {
        Token optoken = token;
        NEXT;
        ExprResult operand = expr12();
        check_unary(&operand, TYPE_INT | TYPE_FLOAT, &optoken);

        if (operand.is_literal) {
            if (operand.data_type == TYPE_INT) {
                operand.int_value = -operand.int_value;
            } else {
                operand.float_value = -operand.float_value;
            }
            return operand;
        } else {
            return unop_expr(operand, UNOP_NEG);
        }
    }

    default:
        return expr12();
    }
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
    ExprResult left = expr9();

    while (token.token_type == TOKEN_KW_MOD)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr9();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT, &optoken);

        if (left.is_literal && right.is_literal) {
            left.int_value %= right.int_value;
        } else {
            left = binop_expr(left, right, BINOP_MOD);
        }
    }
    return left;
}

// + -
static ExprResult expr7() {
    ExprResult left = expr8();

    while (token.token_type == TOKEN_PLUS || token.token_type == TOKEN_MINUS)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr8();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT | TYPE_FLOAT, &optoken);

        if (left.is_literal && right.is_literal) {
            if (left.data_type == TYPE_INT) {
                if (optoken.token_type == TOKEN_PLUS) {
                    left.int_value += right.int_value;
                } else {
                    left.int_value -= right.int_value;
                }
            } else {
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
    ExprResult left = expr7();

    while (token.token_type == TOKEN_CONCAT)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr7();

        check_and_cast(&left, &right, TYPE_STRING, TYPE_STRING, &optoken);

        left = binop_expr(left, right, BINOP_CONCAT);
    }
    return left;
}

// << >>
static ExprResult expr5() {
    ExprResult left = expr6();

    while (token.token_type == TOKEN_LSHIFT || token.token_type == TOKEN_RSHIFT)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr6();

        check_and_cast(&left, &right, TYPE_INT, TYPE_INT, &optoken);

        if (left.is_literal && right.is_literal) {
            if (optoken.token_type == TOKEN_LSHIFT) {
                left.int_value <<= right.int_value;
            } else {
                left.int_value >>= right.int_value;
            }
        } else {
            left = binop_expr(left, right, optoken.token_type == TOKEN_LSHIFT ? BINOP_LSHIFT : BINOP_RSHIFT);
        }
    }
    return left;
}

static const ExprOpType comp_ops_int[] = { BINOP_IEQ, BINOP_INEQ, BINOP_IGT, BINOP_IGTEQ, BINOP_ILS, BINOP_ILSEQ };
static const ExprOpType comp_ops_float[] = { BINOP_FEQ, BINOP_FNEQ, BINOP_FGT, BINOP_FGTEQ, BINOP_FLS, BINOP_FLSEQ };
// = <> > >= < <=
static ExprResult expr4() {
    ExprResult left = expr5();

    while (token.token_type >= TOKEN_EQ && token.token_type <= TOKEN_LSEQ)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr5();

        check_and_cast(&left, &right, TYPE_INT | TYPE_FLOAT, TYPE_INT | TYPE_FLOAT, &optoken);

        if (left.is_literal && right.is_literal) {
            if (left.data_type == TYPE_INT) {
                switch (optoken.token_type)
                {
                case TOKEN_EQ:
                    left.int_value = left.int_value == right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_NEQ:
                    left.int_value = left.int_value != right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_GT:
                    left.int_value = left.int_value > right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_GTEQ:
                    left.int_value = left.int_value >= right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_LS:
                    left.int_value = left.int_value < right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_LSEQ:
                    left.int_value = left.int_value >= right.int_value ? KM_TRUE : KM_FALSE;
                    break;
                }
            } else {
                left.data_type = TYPE_INT;
                switch (optoken.token_type)
                {
                case TOKEN_EQ:
                    left.int_value = left.float_value == right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_NEQ:
                    left.int_value = left.float_value != right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_GT:
                    left.int_value = left.float_value > right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_GTEQ:
                    left.int_value = left.float_value >= right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_LS:
                    left.int_value = left.float_value < right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                case TOKEN_LSEQ:
                    left.int_value = left.float_value >= right.float_value ? KM_TRUE : KM_FALSE;
                    break;
                }
            }
        } else {
            DataType op_type = left.data_type == TYPE_INT ?
                comp_ops_int[optoken.token_type - TOKEN_EQ] :
                comp_ops_float[optoken.token_type - TOKEN_EQ];

            left = binop_expr(left, right, op_type);
        }
    }
    return left;
}

// NOT
static ExprResult expr3() {
    if (token.token_type == TOKEN_KW_NOT) {
        Token optoken = token;
        NEXT;
        ExprResult operand = expr4();
        check_unary(&operand, TYPE_INT, &optoken);

        if (operand.is_literal) {
            operand.int_value = ~operand.int_value;
            return operand;
        } else {
            return unop_expr(operand, UNOP_NOT);
        }
    } else {
        return expr4();
    }
}

// AND ANDALSO
static ExprResult expr2() {
    ExprResult left = expr3();

    while (token.token_type == TOKEN_KW_AND || token.token_type == TOKEN_KW_ANDALSO)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr3();

        check_and_cast(&left, &right, TYPE_INT, TYPE_INT, &optoken);

        if (optoken.token_type == TOKEN_KW_ANDALSO && left.is_literal && left.int_value == 0) continue;

        if (left.is_literal && right.is_literal) {
            left.int_value &= right.int_value;
        } else {
            left = binop_expr(left, right, optoken.token_type == TOKEN_KW_AND ? BINOP_AND : BINOP_ANDSC);
        }
    }
    return left;
}

// OR ORELSE
static ExprResult expr1() {
    ExprResult left = expr2();

    while (token.token_type == TOKEN_KW_OR || token.token_type == TOKEN_KW_ORELSE)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr2();

        check_and_cast(&left, &right, TYPE_INT, TYPE_INT, &optoken);

        if (optoken.token_type == TOKEN_KW_ORELSE && left.is_literal && left.int_value != 0) continue;

        if (left.is_literal && right.is_literal) {
            left.int_value |= right.int_value;
        } else {
            left = binop_expr(left, right, optoken.token_type == TOKEN_KW_OR ? BINOP_OR : BINOP_ORSC);
        }
    }
    return left;
}

// XOR
static ExprResult expr0() {
    ExprResult left = expr1();

    while (token.token_type == TOKEN_KW_XOR)
    {
        Token optoken = token;
        NEXT;
        ExprResult right = expr1();

        check_and_cast(&left, &right, TYPE_INT, TYPE_INT, &optoken);

        if (left.is_literal && right.is_literal) {
            left.int_value ^= right.int_value;
        } else {
            left = binop_expr(left, right, BINOP_XOR);
        }
    }
    return left;
}

static TreeNode* expr() {
    return as_node(expr0());
}

#pragma endregion

void parse(char* _buffer, char* _buffer_end) {
    buffer = _buffer;
    buffer_end = _buffer_end;
    work_data = buffer + sizeof(DictHeader);
    NEXT;
    TreeNode* res = expr();
    printf("Root = %d\n", (uintptr_t)res - (uintptr_t)buffer_end);
    debug_print_tree(buffer_end, _buffer_end);
}