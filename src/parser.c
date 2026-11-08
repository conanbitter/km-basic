#include "common.h"
#include "exptree.h"
#include "lexer.h"
#include "parser.h"
#include "tree.h"
#include "memblock.h"
#include "bytecode.h"

#include <stdbool.h>
#include <stdalign.h>
#include <math.h>

typedef struct ExprResult {
    DataType data_type;
    bool is_const;
    union {
        KmInt int_value;
        KmFloat float_value;
        void* node;
    };
} ExprResult;

#define NEXT next_token(mem_temp, mem_free_end)

#pragma region Parser service functions

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

static DataType get_type_by_name(char* name, size_t name_len) {
    switch (*(name + name_len - 1))
    {
    case '#': return TYPE_FLOAT;
    case '$': return TYPE_STRING;
    default: return TYPE_INT;
    }
}

#pragma endregion

#pragma region Expressions

// SERVICE FUNCTIONS

static void int2float(ExprResult* res) {
    res->data_type = TYPE_FLOAT;
    if (res->is_const) {
        res->float_value = res->int_value;
    } else {
        ExptUnop* node = EXPT_NODE(ExptUnop);
        node->opcode = EXPT_UNOP_ITOF;
        node->arg = expt_addr(node, res->node);

        res->node = node;
    }
}

static void float2int(ExprResult* res) {
    res->data_type = TYPE_INT;
    if (res->is_const) {
        res->int_value = res->float_value;
    } else {
        ExptUnop* node = EXPT_NODE(ExptUnop);
        node->opcode = EXPT_UNOP_FTOI;
        node->arg = expt_addr(node, res->node);

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
            /*res->data_type = TYPE_INT;
            res->is_const = false;
            res->node = mem_add_node();
            res->node->node_type = NODE_DUMMY;*/
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
            /*res->data_type = TYPE_FLOAT;
            res->is_const = false;
            res->node = mem_add_node();
            res->node->node_type = NODE_DUMMY;*/
            break;
        }
        break;

        /*case TYPE_STRING:
            switch (res->data_type)
            {
            case TYPE_INT:
                res->data_type = TYPE_STRING;
                res->is_const = false;
                res->node = mem_add_node();
                res->node->node_type = NODE_DUMMY;
                break;
            case TYPE_FLOAT:
                res->data_type = TYPE_STRING;
                res->is_const = false;
                res->node = mem_add_node();
                res->node->node_type = NODE_DUMMY;
                break;
            }
            break;*/
    }
}

static void* as_node(ExprResult res) {
    if (!res.is_const) return res.node;

    ExptConst* node = EXPT_NODE(ExptConst);
    switch (res.data_type)
    {
    case TYPE_INT:
        node->valtype = TYPE_INT;
        node->value.int_val = res.int_value;
        break;
    case TYPE_FLOAT:
        node->valtype = TYPE_FLOAT;
        node->value.float_val = res.float_value;
        break;
    }

    return node;
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

static ExprResult binop_expr(ExprResult left, ExprResult right, ExptBinopCode opcode) {
    void* left_ptr = as_node(left);
    void* right_ptr = as_node(right);

    ExptBinop* node = EXPT_NODE(ExptBinop);
    node->opcode = opcode;
    node->left = expt_addr(node, left_ptr);
    node->right = expt_addr(node, right_ptr);

    left.is_const = false;
    left.node = node;

    return left;
}

static ExprResult unop_expr(ExprResult operand, ExptUnopCode opcode) {
    void* arg_ptr = as_node(operand);

    ExptUnop* node = EXPT_NODE(ExptUnop);
    node->opcode = opcode;
    node->arg = expt_addr(node, arg_ptr);

    operand.is_const = false;
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
        result.is_const = true;
        result.int_value = token.int_value;
        NEXT;
        return result;

    case TOKEN_FLOATLIT:
        result.data_type = TYPE_FLOAT;
        result.is_const = true;
        result.float_value = token.float_value;
        NEXT;
        return result;

    case TOKEN_STRLIT:
        /*result.data_type = TYPE_STRING;
        result.is_const = false;
        result.node = mem_strlit_node(token.length);
        NEXT;*/
        return result;

    case TOKEN_ID: {
        /*DataType var_type = get_type_by_name(mem_temp, token.length);
        ExptLoad* node = EXPT_NODE(ExptLoad);
        node->datatype = var_type;
        result.data_type = var_type;
        result.is_const = false;
        result.node = node;
        NEXT;
        return result;*/

        NameHeader* id = name_find(token.length);
        if (id == NULL) {
            printf("[%d:%d] ERROR: identifier '%.*s' not found\n",
                token.line,
                token.col,
                token.length,
                mem_temp);
            exit(1);
        }

        switch (id->entry_type)
        {
        case NAME_ENTRY_CONST: {
            ConstBody* body = name_get_body(id);
            result.data_type = body->value_type;
            result.is_const = true;
            switch (body->value_type)
            {
            case TYPE_INT:
                result.int_value = body->value.int_val;
                break;

            case TYPE_FLOAT:
                result.float_value = body->value.float_val;
                break;
            }
            NEXT;
            return result;
        }
        case NAME_ENTRY_VAR:
        case NAME_ENTRY_VAR_LOCAL: {
            VarBody* body = name_get_body(id);

            ExptLoad* node = EXPT_NODE(ExptLoad);
            node->datatype = body->value_type;
            node->local = id->entry_type == NAME_ENTRY_VAR_LOCAL;
            node->addr = (void*)body->offset;

            result.data_type = body->value_type;
            result.is_const = false;
            result.node = node;

            NEXT;
            return result;
        }
        }
    }

    case TOKEN_LPAREN:
        expect(TOKEN_LPAREN);
        result = expr0();
        expect(TOKEN_RPAREN);
        return result;

    case TOKEN_KW_TRUE:
        result.data_type = TYPE_INT;
        result.is_const = true;
        result.int_value = KM_TRUE;
        NEXT;
        return result;

    case TOKEN_KW_FALSE:
        result.data_type = TYPE_INT;
        result.is_const = true;
        result.int_value = KM_FALSE;
        NEXT;
        return result;

    case TOKEN_KW_CINT:
        NEXT;
        expect(TOKEN_LPAREN);
        result = expr0();
        expect(TOKEN_RPAREN);
        type_cast(&result, TYPE_INT);
        return result;

    case TOKEN_KW_CFLOAT:
        NEXT;
        expect(TOKEN_LPAREN);
        result = expr0();
        expect(TOKEN_RPAREN);
        type_cast(&result, TYPE_FLOAT);
        return result;

    case TOKEN_KW_CSTR:
        NEXT;
        expect(TOKEN_LPAREN);
        result = expr0();
        expect(TOKEN_RPAREN);
        type_cast(&result, TYPE_STRING);
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

        if (right.is_const && right.data_type == TYPE_INT && right.int_value < 0) {
            printf("[%d:%d] ERROR: Attempt to exponetiate integer with negative exponent (%" PRIkmINT ").",
                optoken.line,
                optoken.col,
                right.int_value
            );
            exit(1);
        }

        if (left.is_const && right.is_const) {
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

        if (operand.is_const) {
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

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
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
            ExprOpType op_type = left.data_type == TYPE_INT ?
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

        if (operand.is_const) {
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

        if (optoken.token_type == TOKEN_KW_ANDALSO && left.is_const && left.int_value == 0) continue;

        if (left.is_const && right.is_const) {
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

        if (optoken.token_type == TOKEN_KW_ORELSE && left.is_const && left.int_value != 0) continue;

        if (left.is_const && right.is_const) {
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

        if (left.is_const && right.is_const) {
            left.int_value ^= right.int_value;
        } else {
            left = binop_expr(left, right, BINOP_XOR);
        }
    }
    return left;
}

static DataType expr() {
    expt_init();
    ExprResult res = expr0();
    void* root = as_node(res);
    expt_codegen(root);
    expt_finish();
    return res.data_type;
}

static void expr_typed(DataType target_type, Token* stmt) {
    expt_init();
    ExprResult res = expr0();
    type_cast(&res, target_type);

    if (res.data_type != target_type) {
        printf("[%d:%d] ERROR: expression type mismatch. Type is %s, must be %s.",
            stmt->line,
            stmt->col,
            type2str(res.data_type),
            type2str(target_type)
        );
        exit(1);
    }

    void* root = as_node(res);
    expt_codegen(root);
    expt_finish();
}

#pragma endregion

#pragma region Statements

uintptr_t frame_global;
uintptr_t frame_local;
bool in_function = false;

uintptr_t frame_request_global(size_t size) {
    uintptr_t res = frame_global;
    frame_global += size * BYTE_ALIGN;
    return res;
}

static void stmt_const() {
    int stmt_line = token.line;
    int stmt_col = token.col;

    expect(TOKEN_KW_CONST);

    if (token.token_type != TOKEN_ID) unexpected();
    name_check_redecl(token.length, stmt_line, stmt_col);
    DataType value_type = get_type_by_name(mem_temp, token.length);
    name_emplace(token.length, NAME_ENTRY_CONST, stmt_line, stmt_col);

    ConstBody* body = NAME_ALLOC(ConstBody);
    body->value_type = value_type;

    expect(TOKEN_ID);
    expect(TOKEN_EQ);

    stmt_line = token.line;
    stmt_col = token.col;
    ExprResult value = expr0();
    if (!value.is_const) {
        printf("[%d:%d] ERROR: Expression is not constant\n", stmt_line, stmt_col);
        exit(1);
    }
    if (value.data_type != body->value_type) {
        printf("[%d:%d] ERROR: Type mismatch. Constant have type %s, but expression have type %s\n",
            stmt_line,
            stmt_col,
            type2str(body->value_type),
            type2str(value.data_type));
        exit(1);
    }
    switch (value.data_type)
    {
    case TYPE_INT:
        body->value.int_val = value.int_value;
        break;

    case TYPE_FLOAT:
        body->value.float_val = value.float_value;
        break;
    }
}

static void stmt_assign(NameHeader* name) {
    VarBody* body;
    int stmt_line = token.line;
    int stmt_col = token.col;

    if (name == NULL) {
        DataType value_type = get_type_by_name(mem_temp, token.length);
        name_emplace(token.length, NAME_ENTRY_VAR, stmt_line, stmt_col);
        body = NAME_ALLOC(VarBody);
        body->value_type = value_type;
        body->offset = frame_request_global(1);
    } else {
        body = (VarBody*)name_get_body(name);
    }

    expect(TOKEN_ID);
    Token center_token = token;
    expect(TOKEN_EQ);

    expr_typed(body->value_type, &center_token);

    switch (body->value_type)
    {
    case TYPE_INT:
        bc_emit(BC_OPCODE_ISTORE);
        break;

    case TYPE_FLOAT:
        bc_emit(BC_OPCODE_FSTORE);
        break;
    }
    bc_arg_addr((void*)body->offset);
}

static bool stmt() {
    switch (token.token_type)
    {
    case TOKEN_ID: {
        NameHeader* name = name_find(token.length);
        if (name == NULL || name->entry_type == NAME_ENTRY_VAR || name->entry_type == NAME_ENTRY_VAR_LOCAL) {
            stmt_assign(name);
            return true;
        }
    } break;

    default:
        break;
    }
    return false;
}

static void block_main() {
    bool working = true;

    while (working) {
        switch (token.token_type) {
        case TOKEN_KW_CONST:
            stmt_const();
            break;
        default:
            working = stmt();
            break;
        }
        if (!working) break;
        if (token.token_type == TOKEN_EOF) break;
        expect(TOKEN_NEWLINE);
    }
}

#pragma endregion

void parse() {
    NEXT;
    DataType restype;
    block_main();
    bc_debug_print();
}