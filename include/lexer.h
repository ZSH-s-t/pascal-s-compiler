/**
 * @file lexer.h
 * @brief 词法分析器接口定义
 * @author 组员1：词法分析器开发
 * 
 * 本文件定义了词法分析器与语法分析器的接口，
 * 包括Token结构和管理函数。
 * 
 * ===== 后续模块使用指南 =====
 * 
 * 1. 初始化:
 *    init_lexer(filename);
 * 
 * 2. 获取Token:
 *    Token token;
 *    TokenType type = get_next_token(&token);
 * 
 * 3. 预读Token(不消费):
 *    TokenType type = peek_token(&token);
 * 
 * 4. 匹配Token并前进:
 *    if (match_token(&token, TOKEN_BEGIN)) { ... }
 * 
 * 5. 检查错误:
 *    if (has_errors()) { ... }
 * 
 * 6. 关闭:
 *    close_lexer();
 */

#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "error.h"

/**
 * @brief Token属性联合体
 * 
 * 用于存储Token的属性值，如整数值、实数值、字符值或标识符名称
 */
typedef union {
    int int_val;        /* 整数值 */
    double real_val;    /* 实数值 */
    char char_val;      /* 字符值 */
    char str_val[256];  /* 字符串（标识符名或关键字名） */
    char string_val[1024]; /* 字符串常量值 */
} TokenValue;

/**
 * @brief Token结构
 * 
 * 完整的Token信息，包括类型、属性值和位置信息
 */
typedef struct {
    TokenType type;     /* Token类型 */
    TokenValue value;   /* Token属性值 */
    int line;           /* Token所在行号 */
    int column;         /* Token所在列号 */
} Token;

/**
 * @brief 词法分析器状态
 */
typedef struct {
    FILE *input_file;       /* 输入文件 */
    const char *filename;   /* 文件名 */
    int current_line;       /* 当前行号 */
    int current_column;     /* 当前列号 */
    int peeked_char;        /* 预读的字符（用于unread_char） */
    Token current_token;    /* 当前Token */
    int has_peek;          /* 是否已经预读了下一个Token */
    Token peeked_token;     /* 预读的Token */
} LexerState;

/* ========== 词法分析器管理函数 ========== */

/**
 * @brief 初始化词法分析器
 * @param filename 输入文件名
 * @return 成功返回0，失败返回-1
 */
int init_lexer(const char *filename);

/**
 * @brief 关闭词法分析器
 */
void close_lexer(void);

/**
 * @brief 重置词法分析器状态（可用于重新分析）
 */
void reset_lexer(void);

/* ========== Token获取核心接口 ========== */

/**
 * @brief 获取下一个Token（消费式）
 * @param token 用于存储Token信息的结构指针
 * @return 成功返回Token类型，失败返回TOKEN_ERROR
 * 
 * 使用示例:
 *   Token token;
 *   while (get_next_token(&token) != TOKEN_EOF) {
 *       // 处理token
 *   }
 */
TokenType get_next_token(Token *token);

/**
 * @brief 预读下一个Token（不消费）
 * @param token 用于存储Token信息的结构指针
 * @return 成功返回Token类型，失败返回TOKEN_ERROR
 * 
 * 使用示例:
 *   Token token;
 *   if (peek_token(&token) == TOKEN_BEGIN) {
 *       // 可以决定是否消费这个token
 *       // 如果消费，调用get_next_token()
 *   }
 */
TokenType peek_token(Token *token);

/**
 * @brief 确认当前Token（用于断言或跳过检查）
 * @param token 用于存储当前Token的指针
 * @return 当前Token类型
 */
TokenType current_token(Token *token);

/**
 * @brief 跳过当前Token（消费但不处理）
 * @return 被跳过的Token类型
 */
TokenType skip_token(void);

/* ========== Token匹配接口 ========== */

/**
 * @brief 匹配指定类型的Token并消费
 * @param expected 期望的Token类型
 * @return 匹配成功返回1，失败返回0
 * 
 * 使用示例:
 *   if (!expect_token(TOKEN_SEMICOLON)) {
 *       error("Expected ';'");
 *   }
 */
int expect_token(TokenType expected);

/**
 * @brief 匹配指定类型的Token（不消费）
 * @param expected 期望的Token类型
 * @return 匹配成功返回1，失败返回0
 */
int match_token(TokenType expected);

/**
 * @brief 检查当前Token是否为指定类型
 * @param type Token类型
 * @return 是返回1，否返回0
 */
int check_token(TokenType type);

/**
 * @brief 获取Token类型的用户友好名称
 * @param type Token类型
 * @return Token类型的显示名称
 */
const char* get_token_display_name(TokenType type);

/* ========== Token值获取便捷函数 ========== */

/**
 * @brief 获取当前Token的整数值
 * @return 整数值（如果不是整数Token，结果未定义）
 */
int token_int_value(void);

/**
 * @brief 获取当前Token的实数值
 * @return 实数值（如果不是实数Token，结果未定义）
 */
double token_real_value(void);

/**
 * @brief 获取当前Token的字符串值（标识符或字符串常量）
 * @return 字符串指针（如果不是字符串相关Token，结果未定义）
 */
const char* token_string_value(void);

/* ========== 位置信息查询接口 ========== */

/**
 * @brief 获取当前行号
 * @return 当前行号
 */
int get_current_line(void);

/**
 * @brief 获取当前列号
 * @return 当前列号
 */
int get_current_column(void);

/**
 * @brief 获取当前文件名
 * @return 文件名字符串
 */
const char* get_current_filename(void);

/**
 * @brief 获取词法分析器完整状态
 * @return LexerState指针（只读）
 */
const LexerState* get_lexer_state(void);

/* ========== 错误处理接口 ========== */

/**
 * @brief 检查是否有致命错误（导致编译停止的错误）
 * @return 有致命错误返回1，否则返回0
 */
int lexer_has_fatal_errors(void);

/**
 * @brief 获取错误数量
 * @return 当前记录的错误数量
 */
int get_error_count(void);

/* ========== 调试和测试接口 ========== */

/**
 * @brief 打印Token信息（用于调试）
 * @param token Token结构指针
 */
void print_token(const Token *token);

/**
 * @brief 打印Token信息到指定文件
 * @param token Token结构指针
 * @param output 输出文件流
 */
void print_token_to(const Token *token, FILE *output);

/**
 * @brief 词法分析器测试函数
 * 
 * 读取输入文件并输出所有Token，用于测试词法分析器
 */
void test_lexer(void);

#endif /* LEXER_H */
