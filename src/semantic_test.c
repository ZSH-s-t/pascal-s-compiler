/**
 * @file semantic_test.c
 * @brief 语义分析器独立测试程序
 */

#include "semantic.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source.pas>\n", argv[0]);
        return 1;
    }
    
    if (init_lexer(argv[1]) != 0) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return 1;
    }
    
    ASTNode* ast = parse_program();
    
    if (ast && !has_errors()) {
        SemanticResult result = semantic_analyze(ast);
        ast_free(ast);
        close_lexer();
        return result.has_error ? 1 : 0;
    } else {
        fprintf(stderr, "Parsing failed, skipping semantic analysis\n");
        if (ast) ast_free(ast);
        close_lexer();
        return 1;
    }
}