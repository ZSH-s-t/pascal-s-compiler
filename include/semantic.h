/**
 * @file semantic.h
 * @brief 语义分析器头文件
 * @author 陈妍语
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H


#include "ast.h"
#include "symbol.h"

#define LOW_ARRAY_BOUND 1
#define HIGH_ARRAY_BOUND 10

/* 语义分析结果 */
typedef struct {
    int has_error;// 是否有语义错误
    int error_count;// 语义错误数量
} SemanticResult;

/* 表达式类型检查结果 */
typedef struct {
    DataType type;
    int is_const;
    int const_value;
} ExprType;

/*Debug 枚举类型，debug 开头为 1，语义分析器开头为 3*/
typedef enum {
    PRINT_AST = 1301,              // 打印 AST，1301
    DEBUG_CONTENT,                 // 调试内容，1302
} DebugContent;

/* 全局变量：verbose 模式 */
extern int g_verbose;

/* 主入口函数 */
SemanticResult semantic_analyze(ASTNode* ast);

#endif
