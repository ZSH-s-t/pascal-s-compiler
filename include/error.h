/**
 * @file error.h
 * @brief 错误处理模块头文件
 * @author 组员1：词法分析器开发
 * 
 * 本文件定义了编译器的错误处理接口，包括错误类型、
 * 错误记录和错误输出功能。
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

/**
 * @brief 词法分析错误类型
 */
typedef enum {
    LEX_ERROR_NONE = 0,             /* 无错误 */
    LEX_ERROR_ILLEGAL_CHAR,         /* 非法字符 */
    LEX_ERROR_UNTERMINATED_COMMENT, /* 未闭合的注释 */
    LEX_ERROR_INVALID_NUMBER,       /* 无效的数字格式 */
    LEX_ERROR_INVALID_CHAR_CONST,   /* 无效的字符常量 */
    LEX_ERROR_STRING_TOO_LONG,      /* 标识符过长 */
    LEX_ERROR_UNKNOWN               /* 未知错误 */
} LexerErrorType;

/**
 * @brief 错误记录结构
 */
typedef struct {
    LexerErrorType type;    /* 错误类型 */
    int line;               /* 错误行号 */
    int column;             /* 错误列号 */
    char message[256];      /* 错误详细信息 */
} ErrorRecord;

/**
 * @brief 错误列表结构
 */
typedef struct {
    ErrorRecord *records;   /* 错误记录数组 */
    int count;              /* 当前错误数量 */
    int capacity;           /* 数组容量 */
} ErrorList;

/**
 * @brief 全局错误列表（供所有模块使用）
 */
extern ErrorList g_error_list;

/**
 * @brief 初始化错误列表
 */
void init_error_list(void);

/**
 * @brief 释放错误列表资源
 */
void free_error_list(void);

/**
 * @brief 添加错误记录
 * @param type 错误类型
 * @param line 错误行号
 * @param column 错误列号
 * @param format 格式化错误消息
 * @param ... 格式化参数
 */
void add_error(LexerErrorType type, int line, int column, const char *format, ...);

/**
 * @brief 打印所有错误记录
 * @param output 输出文件流（如stdout或stderr）
 */
void print_errors(FILE *output);

/**
 * @brief 获取错误数量
 * @return 当前记录的错误数量
 */
int get_error_count(void);

/**
 * @brief 检查是否有错误
 * @return 如果有错误返回1，否则返回0
 */
int has_errors(void);

/**
 * @brief 清空错误列表
 */
void clear_errors(void);

/**
 * @brief 获取错误类型的描述字符串
 * @param type 错误类型
 * @return 错误类型描述
 */
const char* error_type_to_string(LexerErrorType type);

/**
 * @brief 词法分析器错误报告函数（供Flex调用）
 * @param msg 错误消息
 * @param line 错误行号
 */
void lexer_error(const char *msg, int line);

#endif /* ERROR_H */
void syntax_error(int line, const char *msg);
