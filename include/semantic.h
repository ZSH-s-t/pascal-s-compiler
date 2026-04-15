/**
 * @file semantic.h
 * @brief 语义分析器头文件
 * @author 陈妍语
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol.h"

/* 语义分析结果 */
typedef struct {
    int has_error;
    int error_count;
} SemanticResult;

/* 表达式类型检查结果 */
typedef struct {
    DataType type;
    int is_const;
    int const_value;
} ExprType;

/* 主入口函数 */
SemanticResult semantic_analyze(ASTNode* ast);

#endif