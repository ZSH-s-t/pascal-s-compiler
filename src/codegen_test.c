/**
 * @file codegen_test.c
 * @brief 代码生成器测试程序
 * @author 组员4 & 组员5
 */

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.pas>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        return 1;
    }

    printf("=== Testing Code Generator ===\n");
    printf("Input file: %s\n\n", argv[1]);

    /* 词法分析 */
    lexer_init(input, argv[1]);

    /* 语法分析 */
    printf("=== Parsing ===\n");
    ASTNode *ast = parse_program();
    fclose(input);

    if (!ast) {
        fprintf(stderr, "Error: Parsing failed\n");
        return 1;
    }
    printf("Parsing completed\n\n");

    /* 语义分析 */
    printf("=== Semantic Analysis ===\n");
    SemanticResult sem_result = semantic_analyze(ast);
    
    if (sem_result.has_error) {
        fprintf(stderr, "Error: Semantic analysis failed\n");
        ast_free(ast);
        return 1;
    }
    printf("Semantic analysis completed\n\n");

    /* 代码生成 */
    printf("=== Code Generation ===\n");
    int result = codegen_program(ast, stdout);
    
    if (result != 0) {
        fprintf(stderr, "\nError: Code generation failed\n");
        ast_free(ast);
        return 1;
    }

    printf("\n=== Code generation completed successfully ===\n");

    ast_free(ast);
    return 0;
}
