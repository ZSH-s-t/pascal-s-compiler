/**
 * @file parser.h
 * @brief 语法分析器接口
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/**
 * @brief 解析整个Pascal-S程序，返回AST根节点
 * @return 成功返回AST根节点，失败返回NULL
 */
ASTNode *parse_program(void);

#endif