/**
 * @file parser_test.c
 * @brief 语法分析器独立测试程序
 */

#include "parser.h"
#include "lexer.h"
#include "error.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source.pas>\n", argv[0]);
        return 1;
    }

    if (init_lexer(argv[1]) != 0) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return 1;
    }

    ASTNode *ast = parse_program();

    if (ast && !has_errors()) {
        printf("\n========== Parsing Successful ==========\n");
        ast_print(ast, 0);
        ast_free(ast);
        close_lexer();
        return 0;
    } else {
        printf("\n========== Parsing Failed ==========\n");
        print_errors(stderr);
        if (ast) ast_free(ast);
        close_lexer();
        return 1;
    }
}