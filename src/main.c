/**
 * @file main.c
 * @brief Pascal-S到C语言翻译器主程序
 * @author 全体成员
 * 
 * 使用方法：
 *   ./pasc -i input.pas              # 输出到同目录下的同名.c文件
 *   ./pasc -i input.pas -o output.c  # 指定输出文件
 *   ./pasc -i input.pas -v           # 详细输出模式
 *   ./pasc -h                        # 帮助信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "error.h"

/* 全局变量：verbose 模式 */
int g_verbose = 0;

/**
 * @brief 生成默认的输出文件名（替换扩展名为.c）
 * @param input_file 输入文件名
 * @return 动态分配的输出文件名字符串，调用者需手动释放
 */
static char* generate_output_filename(const char *input_file) {
    if (!input_file) return NULL;
    
    size_t len = strlen(input_file);
    char *output = (char*)malloc(len + 5);  /* 为 ".c" 和结束符预留空间 */
    if (!output) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    
    strcpy(output, input_file);
    
    /* 查找最后一个点号 */
    char *dot = strrchr(output, '.');
    if (dot && (strcmp(dot, ".pas") == 0 || strcmp(dot, ".PAS") == 0)) {
        strcpy(dot, ".c");
    } else {
        strcat(output, ".c");
    }
    
    return output;
}

/**
 * @brief 打印使用帮助
 */
static void print_usage(const char *program_name) {
    printf("Pascal-S to C Translator\n");
    printf("Usage: %s -i <input.pas> [-o <output.c>] [-v] [-h]\n\n", program_name);
    printf("Options:\n");
    printf("  -i <file>    Input Pascal-S source file (required)\n");
    printf("  -o <file>    Output C source file (default: <input>.c)\n");
    printf("  -v           Verbose mode\n");
    printf("  -h           Display this help message\n\n");
    printf("Examples:\n");
    printf("  %s -i test.pas\n", program_name);
    printf("  %s -i test.pas -o output.c\n", program_name);
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[]) {
    const char *input_file = NULL;
    char *output_file = NULL;
    int verbose = 0;
    int auto_output = 0;  /* 标记输出文件名是否为自动生成（需手动释放） */

    /* ========== 1. 解析命令行参数 ========== */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            input_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = (char*)argv[++i];
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* 检查必须参数 */
    if (!input_file) {
        fprintf(stderr, "Error: Input file required (-i <file>)\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* 设置全局 verbose 变量 */
    g_verbose = verbose;

    /* 生成默认输出文件名 */
    if (!output_file) {
        output_file = generate_output_filename(input_file);
        if (!output_file) return 1;
        auto_output = 1;
    }

    if (verbose) {
        printf("========================================\n");
        printf("  Pascal-S to C Translator\n");
        printf("========================================\n");
        printf("Input file:  %s\n", input_file);
        printf("Output file: %s\n\n", output_file);
    }

    /* ========== 2. 初始化词法分析器 ========== */
    if (verbose) printf("--- Phase 1: Lexical Analysis ---\n");
    
    if (init_lexer(input_file) != 0) {
        fprintf(stderr, "Error: Failed to open input file '%s'\n", input_file);
        if (auto_output) free(output_file);
        return 1;
    }
    if (verbose) printf("Lexer initialized successfully.\n\n");

    /* ========== 3. 语法分析 ========== */
    if (verbose) printf("--- Phase 2: Syntax Analysis ---\n");
    
    ASTNode *ast = parse_program();
    
    if (!ast) {
        fprintf(stderr, "Error: Syntax analysis failed\n");
        close_lexer();
        if (auto_output) free(output_file);
        return 1;
    }
    if (verbose) printf("Syntax analysis completed.\n\n");

    /* ========== 4. 语义分析 ========== */
    if (verbose) printf("--- Phase 3: Semantic Analysis ---\n");
    
    SemanticResult sem_result = semantic_analyze(ast);
    
    if (sem_result.has_error) {
        fprintf(stderr, "Semantic analysis found %d error(s)\n", sem_result.error_count);
        /* 不中止，继续尝试代码生成 */
    } else {
        if (verbose) printf("Semantic analysis completed successfully.\n\n");
    }

    /* ========== 5. 代码生成 ========== */
    if (verbose) printf("--- Phase 4: Code Generation ---\n");
    
    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        ast_free(ast);
        close_lexer();
        if (auto_output) free(output_file);
        return 1;
    }

    int codegen_result = codegen_program(ast, output);
    fclose(output);

    if (codegen_result != 0) {
        fprintf(stderr, "Warning: Code generation completed with errors\n");
    } else {
        if (verbose) printf("Code generation completed successfully.\n\n");
    }

    /* ========== 6. 清理和退出 ========== */
    ast_free(ast);
    close_lexer();

    if (verbose) {
        printf("========================================\n");
        printf("  Translation completed: %s -> %s\n", input_file, output_file);
        if (sem_result.has_error) {
            printf("  %d semantic error(s) found\n", sem_result.error_count);
        }
        printf("========================================\n");
    } else {
        printf("%s -> %s\n", input_file, output_file);
    }

    if (auto_output) free(output_file);
    free_symtable();
    return sem_result.has_error ? 1 : 0;
}