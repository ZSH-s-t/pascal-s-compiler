/**
 * @file main.c
 * @brief Pascal-S到C语言翻译器主程序
 * @author 全体成员
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "error.h"
#include "lexer.h"
#include "parser.h" 
#include "semantic.h"
#include "codegen.h"

/**
 * @brief 打印使用帮助
 */
void print_usage(const char *program_name) {
    printf("Pascal-S to C Translator (pascc)\n");
    printf("Usage: %s -i <input.pas> [-o <output.c>] [options]\n\n", program_name);
    printf("Options:\n");
    printf("  -i <file>       Input Pascal-S source file (required)\n");
    printf("  -o <file>       Output C source file (default: <input>.c)\n");
    printf("  -v, --verbose   Verbose output mode\n");
    printf("  -h, --help      Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s -i test.pas\n", program_name);
    printf("  %s -i test.pas -o output.c\n", program_name);
}

/**
 * @brief 生成输出文件名
 */
char* generate_output_filename(const char *input_file) {
    char *output = malloc(strlen(input_file) + 10);
    strcpy(output, input_file);
    
    /* 查找.pas扩展名 */
    char *dot = strrchr(output, '.');
    if (dot && (strcmp(dot, ".pas") == 0 || strcmp(dot, ".PAS") == 0)) {
        strcpy(dot, ".c");
    } else {
        strcat(output, ".c");
    }
    
    return output;
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[]) {
    char *input_file = NULL;
    char *output_file = NULL;
    int verbose = 0;
    int auto_output = 0;
    
    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    /* 检查必需参数 */
    if (!input_file) {
        fprintf(stderr, "Error: Input file required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* 生成输出文件名 */
    if (!output_file) {
        output_file = generate_output_filename(input_file);
        auto_output = 1;
    }
    
    if (verbose) {
        printf("Input file: %s\n", input_file);
        printf("Output file: %s\n", output_file);
        printf("\n");
    }
    
    /* 打开输入文件 */
    FILE *input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        if (auto_output) free(output_file);
        return 1;
    }
    
    /* 错误收集标志 */
    int has_errors = 0;
    int total_errors = 0;
    
    /* 初始化词法分析器 */
    if (verbose) printf("=== Phase 1: Lexical Analysis ===\n");
    if (init_lexer(input_file) != 0) {
        fprintf(stderr, "Error: Failed to initialize lexer\n");
        return 1;
    }
    
    /* 语法分析 - 即使失败也继续 */
    if (verbose) printf("=== Phase 2: Syntax Analysis ===\n");
    ASTNode *ast = parse_program();
    
    fclose(input);
    
    if (!ast) {
        if (verbose) fprintf(stderr, "Warning: Syntax analysis encountered errors\n");
        has_errors = 1;
        /* 不立即返回，继续处理 */
    } else {
        if (verbose) {
            printf("Syntax analysis completed successfully\n\n");
        }
    }
    
    /* 语义分析 - 即使有语法错误也尝试（如果有AST） */
    if (verbose) printf("=== Phase 3: Semantic Analysis ===\n");
    SemanticResult sem_result = {0, 0};
    
    if (ast) {
        sem_result = semantic_analyze(ast);
        
        if (sem_result.has_error) {
            if (verbose) fprintf(stderr, "Warning: Semantic analysis found %d error(s)\n", 
                    sem_result.error_count);
            has_errors = 1;
            total_errors += sem_result.error_count;
            /* 不立即返回，继续处理 */
        } else {
            if (verbose) {
                printf("Semantic analysis completed successfully\n\n");
            }
        }
    } else {
        if (verbose) fprintf(stderr, "Skipping semantic analysis due to syntax errors\n\n");
    }
    
    /* 代码生成 - 即使有错误也尝试生成（如果有AST） */
    if (verbose) printf("=== Phase 4: Code Generation ===\n");
    
    if (ast) {
        FILE *output = fopen(output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
            ast_free(ast);
            if (auto_output) free(output_file);
            return 1;
        }
        
        int codegen_result = codegen_program(ast, output);
        fclose(output);
        
        if (codegen_result != 0) {
            if (verbose) fprintf(stderr, "Warning: Code generation encountered errors\n");
            has_errors = 1;
            /* 不立即返回 */
        } else {
            if (verbose) {
                printf("Code generation completed successfully\n");
            }
        }
    } else {
        if (verbose) fprintf(stderr, "Skipping code generation due to syntax errors\n");
    }
    
    /* 输出最终结果 */
    if (has_errors) {
        fprintf(stderr, "\n=== Translation completed with errors ===\n");
        fprintf(stderr, "Total errors found: %d\n", total_errors);
        fprintf(stderr, "Output file may be incomplete: %s\n", output_file);
    } else {
        if (verbose) {
            printf("\n=== Translation completed successfully ===\n");
            printf("Output written to: %s\n", output_file);
        } else {
            printf("Translation completed: %s -> %s\n", input_file, output_file);
        }
    }
    
    /* 清理 */
    if (ast) ast_free(ast);
    if (auto_output) free(output_file);
    
    /* 返回状态：有错误返回1，无错误返回0 */
    return has_errors ? 1 : 0;
}
