/**
 * @file codegen.h
 * @brief 代码生成器头文件
 * @author 组员4 & 组员5
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol.h"
#include <stdio.h>

/* 代码生成器上下文 */
typedef struct {
    FILE *output;           /* 输出文件 */
    int indent_level;       /* 缩进级别 */
    int temp_var_count;     /* 临时变量计数 */
    int label_count;        /* 标签计数 */
    const char *current_function; /* 当前函数名（用于处理函数返回值） */
} CodeGenContext;

/* 初始化代码生成器 */
void codegen_init(CodeGenContext *ctx, FILE *output);

/* 生成完整的C程序 */
int codegen_program(ASTNode *ast, FILE *output);

/* 生成声明部分 */
void codegen_declarations(CodeGenContext *ctx, ASTNode *const_decls, 
                          ASTNode *var_decls, ASTNode *subprog_decls);

/* 生成语句 */
void codegen_statement(CodeGenContext *ctx, ASTNode *stmt);

/* 生成表达式 */
void codegen_expression(CodeGenContext *ctx, ASTNode *expr);

/* 辅助函数 */
void codegen_indent(CodeGenContext *ctx);
const char* get_c_type(DataType type);
const char* get_c_operator(BinaryOp op);

#endif
