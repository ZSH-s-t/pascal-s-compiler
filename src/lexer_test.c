/**
 * @file main.c
 * @brief 词法分析器测试主程序
 * @author 组员1：词法分析器开发
 
 * 用于独立测试词法分析器功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "error.h"
#include "lexer.h"
#include "parser.h" 
#include "semantic.h"



/**
 * @brief 打印使用帮助
 */
void print_usage(const char *program_name) {
    printf("Pascal-S Lexer Test Program\n");
    printf("Usage: %s <source_file.pas> [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -v, --verbose   Verbose output mode\n");
    printf("  -h, --help      Display this help message\n");
    printf("  -o, --output    Specify output file (default: stdout)\n");
    printf("\nExamples:\n");
    printf("  %s test.pas\n", program_name);
    printf("  %s test.pas -v\n", program_name);
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[]) {
    char *filename = NULL;
    int verbose = 0;
    FILE *output = stdout;
    
    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output = fopen(argv[++i], "w");
                if (output == NULL) {
                    fprintf(stderr, "Error: Cannot open output file '%s'\n", argv[i]);
                    return 1;
                }
            } else {
                fprintf(stderr, "Error: -o option requires output filename\n");
                return 1;
            }
        }
        else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* 初始化词法分析器 */
    if (init_lexer(filename) != 0) {
        return 1;
    }
    
    if (verbose) {
        printf("Analyzing file: %s\n\n", filename);
    }
    
    /* 运行词法分析器测试 */
    test_lexer();
    
    /* 关闭词法分析器 */
    close_lexer();
    
    if (output != stdout) {
        fclose(output);
    }
    
    return has_errors() ? 1 : 0;


    // 2. 语法分析阶段
    printf("\n=== 2. 语法分析阶段 ===\n");
    ASTNode* ast = parse_program();
    
    if (ast == NULL || has_errors()) {
        fprintf(stderr, "Syntax analysis failed. Aborting.\n");
        close_lexer();
        return 1;
    }
    printf("Syntax analysis completed successfully.\n");
    
    // 3. 语义分析阶段
    printf("\n=== 3. 语义分析阶段 ===\n");
    SemanticResult sem_result = semantic_analyze(ast);
    
    if (sem_result.has_error) {
        fprintf(stderr, "Semantic analysis failed with %d error(s).\n", 
                sem_result.error_count);
        ast_free(ast);
        close_lexer();
        return 1;
    }
    printf("Semantic analysis completed successfully.\n");
}
