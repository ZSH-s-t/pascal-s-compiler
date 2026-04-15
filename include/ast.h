/**
 * @file ast.h
 * @brief 抽象语法树（AST）节点定义
 */

#ifndef AST_H
#define AST_H

#include "token.h"

/* AST节点类型 */
typedef enum {
    AST_PROGRAM,        /* 程序根 */
    AST_VAR_DECL,       /* 变量声明 */
    AST_CONST_DECL,     /* 常量声明 */
    AST_TYPE_DECL,      /* 类型声明（integer/real/array）*/
    AST_SUBPROG_DECL,   /* 函数/过程声明 */
    AST_COMPOUND_STMT,  /* begin...end */
    AST_ASSIGN_STMT,    /* 赋值 := */
    AST_IF_STMT,        /* if-then-else */
    AST_FOR_STMT,       /* for循环 */
    AST_WHILE_STMT,     /* while循环（扩展）*/
    AST_CALL_STMT,      /* 过程调用 */
    AST_READ_STMT,      /* read语句 */
    AST_WRITE_STMT,     /* write/writeln语句 */
    AST_BINARY_EXPR,    /* 二元运算 */
    AST_UNARY_EXPR,     /* 一元运算 */
    AST_VAR_REF,        /* 变量引用（含数组下标）*/
    AST_CONST_VAL,      /* 常量值 */
    AST_IDENTIFIER,     /* 标识符（用于声明列表）*/
    AST_PARAM_LIST,     /* 参数列表 */
    AST_STMT_LIST       /* 语句列表 */
} ASTNodeType;

/* 二元运算符 */
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV,
    OP_DIV_INT, OP_MOD,          /* div, mod */
    OP_AND, OP_OR,               /* and, or */
    OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE
} BinaryOp;

/* 一元运算符 */
typedef enum {
    UNARY_NOT, UNARY_MINUS
} UnaryOp;

/* 数据类型 */
typedef enum {
    TYPE_UNKNOWN = -1,
    TYPE_INTEGER, TYPE_REAL, TYPE_BOOLEAN, TYPE_CHAR,
    TYPE_ARRAY
} DataType;

/* 前向声明 */
typedef struct ASTNode ASTNode;

/* AST节点结构 */
struct ASTNode {
    ASTNodeType type;
    int line;                       /* 源代码行号 */
    union {
        /* 程序 */
        struct {
            char prog_name[64];
            ASTNode *const_decls;
            ASTNode *var_decls;
            ASTNode *subprog_decls;
            ASTNode *body;
        } program;

        /* 变量声明 */
        struct {
            ASTNode *id_list;       /* AST_IDENTIFIER链表 */
            DataType data_type;
            ASTNode *array_bounds;  /* 数组下标范围表达式链表 */
            ASTNode *next_decl;
        } var_decl;

        /* 常量声明 */
        struct {
            char name[64];
            ASTNode *value;
            ASTNode *next_decl;
        } const_decl;

        /* 子程序声明 */
        struct {
            int is_function;
            char name[64];
            ASTNode *params;
            DataType return_type;
            ASTNode *body;
            ASTNode *next_decl;
        } subprog;

        /* 复合语句 */
        struct {
            ASTNode *stmt_list;
        } compound;

        /* 赋值 */
        struct {
            ASTNode *lhs;
            ASTNode *rhs;
        } assign;

        /* if */
        struct {
            ASTNode *cond;
            ASTNode *then_part;
            ASTNode *else_part;
        } if_stmt;

        /* for */
        struct {
            char var_name[64];
            ASTNode *start_expr;
            ASTNode *end_expr;
            ASTNode *body;
        } for_stmt;

        /* 二元表达式 */
        struct {
            BinaryOp op;
            ASTNode *left;
            ASTNode *right;
        } binary;

        /* 一元表达式 */
        struct {
            UnaryOp op;
            ASTNode *operand;
        } unary;

        /* 变量引用 */
        struct {
            char name[64];
            ASTNode *index_expr;
        } var_ref;

        /* 常量值 */
        struct {
            TokenType token_type;
            union {
                int int_val;
                double real_val;
                char char_val;
                int bool_val;
            };
        } const_val;

        /* 语句列表 */
        struct {
            ASTNode *first;
            ASTNode *last;
        } stmt_list;

        /* 标识符链表节点 */
        struct {
            char name[64];
            ASTNode *next;
        } id_node;

        /* 参数列表项 */
        struct {
            int is_var;         /* 是否为var参数 */
            ASTNode *id_list;
            DataType type;
            ASTNode *next_param;
        } param;
    } data;

    ASTNode *next;      /* 通用链表指针 */
};

/* AST 构造函数 */
ASTNode *ast_new_node(ASTNodeType type, int line);
ASTNode *ast_new_identifier(const char *name, int line);
ASTNode *ast_new_const_int(int val, int line);
ASTNode *ast_new_const_real(double val, int line);
ASTNode *ast_new_const_char(char val, int line);
ASTNode *ast_new_const_bool(int val, int line);
ASTNode *ast_new_binary_expr(BinaryOp op, ASTNode *left, ASTNode *right, int line);
ASTNode *ast_new_unary_expr(UnaryOp op, ASTNode *operand, int line);
ASTNode *ast_new_var_ref(const char *name, ASTNode *index, int line);

/* AST 工具函数 */
ASTNode *ast_append_stmt(ASTNode *list, ASTNode *stmt);
ASTNode *ast_append_decl(ASTNode *list, ASTNode *decl);
ASTNode *ast_append_id(ASTNode *list, const char *name, int line);
ASTNode *ast_append_param(ASTNode *list, int is_var, ASTNode *id_list, DataType type, int line);

/* 打印AST（调试用） */
void ast_print(ASTNode *node, int indent);

/* 释放AST */
void ast_free(ASTNode *node);

#endif