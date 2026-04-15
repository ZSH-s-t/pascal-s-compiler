/**
 * @file error.c
 * @brief 错误处理模块实现
 * @author 组员1：词法分析器开发
 * 
 * 本文件实现了编译器的错误处理功能，包括错误记录和输出。
 */

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* 错误列表容量初始值 */
#define ERROR_LIST_INITIAL_CAPACITY 16

/* 全局错误列表 */
ErrorList g_error_list;

/* 关键字表（用于错误消息中显示关键字名） */
static const char* keyword_names[] = {
    "program", "var", "const", "procedure", "function",
    "begin", "end", "if", "then", "else",
    "for", "to", "do", "while", "repeat",
    "until", "case", "read", "write", "writeln",
    "array", "of", "integer", "real", "boolean",
    "char", "div", "mod", "and", "or",
    "not", "true", "false"
};

/**
 * @brief 初始化错误列表
 */
void init_error_list(void) {
    g_error_list.records = (ErrorRecord*)malloc(
        sizeof(ErrorRecord) * ERROR_LIST_INITIAL_CAPACITY);
    if (g_error_list.records == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for error list\n");
        exit(1);
    }
    
    g_error_list.count = 0;
    g_error_list.capacity = ERROR_LIST_INITIAL_CAPACITY;
}

/**
 * @brief 释放错误列表资源
 */
void free_error_list(void) {
    if (g_error_list.records != NULL) {
        free(g_error_list.records);
        g_error_list.records = NULL;
    }
    g_error_list.count = 0;
    g_error_list.capacity = 0;
}

/**
 * @brief 添加错误记录
 */
void add_error(LexerErrorType type, int line, int column, const char *format, ...) {
    /* 必要时扩展错误列表 */
    if (g_error_list.count >= g_error_list.capacity) {
        int new_capacity = g_error_list.capacity * 2;
        ErrorRecord *new_records = (ErrorRecord*)realloc(
            g_error_list.records, sizeof(ErrorRecord) * new_capacity);
        
        if (new_records == NULL) {
            fprintf(stderr, "Fatal: Memory allocation failed for error list expansion\n");
            return;
        }
        
        g_error_list.records = new_records;
        g_error_list.capacity = new_capacity;
    }
    
    /* 填充错误记录 */
    ErrorRecord *record = &g_error_list.records[g_error_list.count];
    record->type = type;
    record->line = line;
    record->column = column;
    
    /* 格式化错误消息 */
    va_list args;
    va_start(args, format);
    vsnprintf(record->message, sizeof(record->message), format, args);
    va_end(args);
    
    g_error_list.count++;
    
    /* 同时输出到stderr */
    fprintf(stderr, "[Lexer Error] Line %d, Column %d: %s\n", 
            line, column, record->message);
}

/**
 * @brief 打印所有错误记录
 */
void print_errors(FILE *output) {
    if (output == NULL) {
        output = stderr;
    }
    
    if (g_error_list.count == 0) {
        fprintf(output, "No errors detected.\n");
        return;
    }
    
    fprintf(output, "\n========== Error Summary ==========\n");
    fprintf(output, "Total errors: %d\n\n", g_error_list.count);
    
    for (int i = 0; i < g_error_list.count; i++) {
        ErrorRecord *record = &g_error_list.records[i];
        fprintf(output, "[%d] Line %d, Column %d: %s\n",
                i + 1,
                record->line,
                record->column,
                record->message);
    }
    
    fprintf(output, "=====================================\n");
}

/**
 * @brief 获取错误数量
 */
int get_error_count(void) {
    return g_error_list.count;
}

/**
 * @brief 检查是否有错误
 */
int has_errors(void) {
    return g_error_list.count > 0;
}

/**
 * @brief 清空错误列表
 */
void clear_errors(void) {
    g_error_list.count = 0;
}

/**
 * @brief 获取错误类型的描述字符串
 */
const char* error_type_to_string(LexerErrorType type) {
    switch (type) {
        case LEX_ERROR_NONE:
            return "No Error";
        case LEX_ERROR_ILLEGAL_CHAR:
            return "Illegal Character";
        case LEX_ERROR_UNTERMINATED_COMMENT:
            return "Unterminated Comment";
        case LEX_ERROR_INVALID_NUMBER:
            return "Invalid Number Format";
        case LEX_ERROR_INVALID_CHAR_CONST:
            return "Invalid Character Constant";
        case LEX_ERROR_STRING_TOO_LONG:
            return "String Too Long";
        default:
            return "Unknown Error";
    }
}

/**
 * @brief 词法分析器错误报告函数（供其他模块调用）
 */
void lexer_error(const char *msg, int line) {
    add_error(LEX_ERROR_UNKNOWN, line, 0, "%s", msg);
}

void syntax_error(int line, const char *msg) {
    fprintf(stderr, "[Syntax Error] Line %d: %s\n", line, msg);
}
