#ifndef TREE_H
#define TREE_H


typedef enum NodeType {
    NODE_BINOP,
    NODE_INTLIT,
    NODE_FLOATLIT,
} NodeType;

typedef enum BinOpType {
    BINOP_POWER,
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
    BINOP_OR,
    BINOP_XOR
} BinOpType;

typedef struct TreeNode TreeNode;

typedef struct BinOpData {
    TreeNode* left;
    TreeNode* right;
    BinOpType op;
} BinOpData;

struct TreeNode
{
    union {
        BinOpData binop;
        KmInt intlit;
        KmFloat floatlit;
    };
    NodeType node_type;
};


#endif