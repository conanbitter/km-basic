#ifndef TREE_H
#define TREE_H

#include "common.h"
#include "stdbool.h"

typedef enum NodeType {
    NODE_EXPROP,
    NODE_INTLIT,
    NODE_FLOATLIT,
    NODE_LOAD,
} NodeType;

typedef enum ExprOpType {
    BINOP_IPOWER,
    BINOP_FPOWER,
    BINOP_IMUL,
    BINOP_FMUL,
    BINOP_IDIV,
    BINOP_FDIV,
    BINOP_MOD,
    BINOP_IADD,
    BINOP_FADD,
    BINOP_ISUB,
    BINOP_FSUB,
    BINOP_CONCAT,
    BINOP_LSHIFT,
    BINOP_RSHIFT,
    BINOP_IEQ,
    BINOP_FEQ,
    BINOP_SEQ,
    BINOP_INEQ,
    BINOP_FNEQ,
    BINOP_SNEQ,
    BINOP_IGT,
    BINOP_FGT,
    BINOP_IGTEQ,
    BINOP_FGTEQ,
    BINOP_ILS,
    BINOP_FLS,
    BINOP_ILSEQ,
    BINOP_FLSEQ,
    BINOP_AND,
    BINOP_ANDSC, // short-circuit
    BINOP_OR,
    BINOP_ORSC,  // short-circuit
    BINOP_XOR,
    UNOP_NEG,
    UNOP_NOT,
    UNOP_ITOF,
    UNOP_FTOI
} ExprOpType;

typedef struct TreeNode TreeNode;

typedef struct ExprOpData {
    TreeNode* left;
    TreeNode* right;
    ExprOpType op;
} ExprOpData;

typedef struct LoadData {
    uintptr_t offset;
    bool is_local;
} LoadData;

struct TreeNode
{
    union {
        ExprOpData exprop;
        KmInt intlit;
        KmFloat floatlit;
        LoadData load;
        TreeNode* child;
    };
    NodeType node_type;
};

void debug_print_tree(char* start, char* end);

#endif