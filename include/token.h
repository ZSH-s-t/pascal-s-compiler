/**
 * @file token.h
 * @brief Pascal-S 语言 Token 类型定义
 * @author 组员1：词法分析器开发
 * 
 * 本文件定义了Pascal-S语言中所有Token类型的枚举，
 * 供词法分析器和语法分析器使用。
 */

#ifndef TOKEN_H
#define TOKEN_H

/**
 * @brief Token类型枚举
 * 
 * 包含关键字、标识符、常量、运算符、分隔符等所有Token类型
 */
typedef enum {
    /* ========== 错误和结束标记 ========== */
    TOKEN_ERROR = 0,        /* 错误Token */
    TOKEN_EOF,              /* 文件结束 */
    
    /* ========== 关键字（保留字）========== */
    TOKEN_PROGRAM,          /* program */
    TOKEN_VAR,              /* var */
    TOKEN_CONST,            /* const */
    TOKEN_PROCEDURE,        /* procedure */
    TOKEN_FUNCTION,         /* function */
    TOKEN_BEGIN,            /* begin */
    TOKEN_END,              /* end */
    TOKEN_IF,               /* if */
    TOKEN_THEN,             /* then */
    TOKEN_ELSE,             /* else */
    TOKEN_FOR,              /* for */
    TOKEN_TO,               /* to */
    TOKEN_DO,               /* do */
    TOKEN_WHILE,            /* while */
    TOKEN_REPEAT,           /* repeat */
    TOKEN_UNTIL,            /* until */
    TOKEN_CASE,             /* case */
    TOKEN_READ,             /* read */
    TOKEN_WRITE,            /* write */
    TOKEN_WRITELN,          /* writeln */
    TOKEN_ARRAY,            /* array */
    TOKEN_OF,               /* of */
    TOKEN_INTEGER,          /* integer */
    TOKEN_REAL,             /* real */
    TOKEN_BOOLEAN,          /* boolean */
    TOKEN_CHAR,             /* char */
    TOKEN_DIV,              /* div */
    TOKEN_MOD,              /* mod */
    TOKEN_AND,              /* and */
    TOKEN_OR,               /* or */
    TOKEN_NOT,              /* not */
    TOKEN_TRUE,             /* true */
    TOKEN_FALSE,            /* false */
    
    /* ========== 标识符和常量 ========== */
    TOKEN_IDENTIFIER,       /* 标识符 */
    TOKEN_INTEGER_CONST,    /* 整型常量 */
    TOKEN_REAL_CONST,       /* 实型常量 */
    TOKEN_CHAR_CONST,       /* 字符常量 */
    TOKEN_STRING_CONST,     /* 字符串常量 */
    
    /* ========== 运算符 ========== */
    /* 算术运算符 */
    TOKEN_PLUS,             /* + */
    TOKEN_MINUS,            /* - */
    TOKEN_MULTIPLY,         /* * */
    TOKEN_DIVIDE,           /* / */
    
    /* 关系运算符 */
    TOKEN_EQ,               /* = */
    TOKEN_NE,               /* <> */
    TOKEN_LT,               /* < */
    TOKEN_LE,               /* <= */
    TOKEN_GT,               /* > */
    TOKEN_GE,               /* >= */
    
    /* 赋值运算符 */
    TOKEN_ASSIGN,           /* := */
    
    /* ========== 分隔符 ========== */
    TOKEN_SEMICOLON,        /* ; */
    TOKEN_COMMA,            /* , */
    TOKEN_DOT,              /* . */
    TOKEN_DOUBLEDOT,        /* .. */
    TOKEN_LPAREN,           /* ( */
    TOKEN_RPAREN,           /* ) */
    TOKEN_LBRACKET,         /* [ */
    TOKEN_RBRACKET,         /* ] */
    TOKEN_COLON,            /* : */
    
    /* ========== Token类型总数 ========== */
    TOKEN_COUNT
} TokenType;

/**
 * @brief 关键字查找表结构
 */
typedef struct {
    const char *name;       /* 关键字名称 */
    TokenType type;         /* 对应的Token类型 */
} KeywordEntry;

/**
 * @brief 获取Token类型的字符串表示
 * @param type Token类型
 * @return Token类型的字符串名称
 */
const char* token_type_to_string(TokenType type);

/**
 * @brief 根据字符串查找关键字Token类型
 * @param name 要查找的字符串（不区分大小写）
 * @return 如果是关键字返回对应的Token类型，否则返回TOKEN_ERROR
 */
TokenType lookup_keyword(const char *name);

/**
 * @brief 判断Token是否为关键字
 * @param type Token类型
 * @return 如果是关键字返回1，否则返回0
 */
int is_keyword(TokenType type);

/**
 * @brief 初始化关键字查找表
 */
void init_keyword_table(void);

#endif /* TOKEN_H */
