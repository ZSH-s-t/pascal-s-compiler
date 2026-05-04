/**
 * @file token.c
 * @brief Token类型相关函数实现
 * @author 组员1：词法分析器开发
 */

#include "token.h"
#include <string.h>
#include <ctype.h>

/* 不依赖 strcasecmp（部分 C99 环境未声明） */
static int keyword_name_iequal(const char *a, const char *b) {
    if (!a || !b) return a == b ? 0 : 1;
    for (; *a && *b; a++, b++) {
        int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (d) return d;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/* 关键字查找表 */
static KeywordEntry keyword_table[] = {
    /* 程序结构关键字 */
    {"program", TOKEN_PROGRAM},
    {"var", TOKEN_VAR},
    {"const", TOKEN_CONST},
    {"procedure", TOKEN_PROCEDURE},
    {"function", TOKEN_FUNCTION},
    {"begin", TOKEN_BEGIN},
    {"end", TOKEN_END},
    
    /* 控制结构关键字 */
    {"if", TOKEN_IF},
    {"then", TOKEN_THEN},
    {"else", TOKEN_ELSE},
    {"for", TOKEN_FOR},
    {"to", TOKEN_TO},
    {"do", TOKEN_DO},
    {"while", TOKEN_WHILE},
    {"repeat", TOKEN_REPEAT},
    {"until", TOKEN_UNTIL},
    {"case", TOKEN_CASE},
    
    /* 输入输出关键字 */
    {"readln", TOKEN_READLN},
    {"read", TOKEN_READ},
    {"write", TOKEN_WRITE},
    {"writeln", TOKEN_WRITELN},
    
    /* 类型关键字 */
    {"array", TOKEN_ARRAY},
    {"of", TOKEN_OF},
    {"integer", TOKEN_INTEGER},
    {"real", TOKEN_REAL},
    {"boolean", TOKEN_BOOLEAN},
    {"char", TOKEN_CHAR},
    
    /* 运算符关键字 */
    {"div", TOKEN_DIV},
    {"mod", TOKEN_MOD},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    
    /* 布尔常量关键字 */
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    
    /* 结束标记 */
    {NULL, TOKEN_ERROR}  /* 表结束标记 */
};

/* 关键字表是否已初始化 */
static int keyword_table_initialized = 0;

/**
 * @brief 初始化关键字查找表
 */
void init_keyword_table(void) {
    keyword_table_initialized = 1;
}

/**
 * @brief 获取Token类型的字符串表示
 */
const char* token_type_to_string(TokenType type) {
    switch (type) {
        /* 错误和结束标记 */
        case TOKEN_ERROR: return "TOKEN_ERROR";
        case TOKEN_EOF: return "TOKEN_EOF";
        
        /* 关键字 */
        case TOKEN_PROGRAM: return "TOKEN_PROGRAM";
        case TOKEN_VAR: return "TOKEN_VAR";
        case TOKEN_CONST: return "TOKEN_CONST";
        case TOKEN_PROCEDURE: return "TOKEN_PROCEDURE";
        case TOKEN_FUNCTION: return "TOKEN_FUNCTION";
        case TOKEN_BEGIN: return "TOKEN_BEGIN";
        case TOKEN_END: return "TOKEN_END";
        case TOKEN_IF: return "TOKEN_IF";
        case TOKEN_THEN: return "TOKEN_THEN";
        case TOKEN_ELSE: return "TOKEN_ELSE";
        case TOKEN_FOR: return "TOKEN_FOR";
        case TOKEN_TO: return "TOKEN_TO";
        case TOKEN_DO: return "TOKEN_DO";
        case TOKEN_WHILE: return "TOKEN_WHILE";
        case TOKEN_REPEAT: return "TOKEN_REPEAT";
        case TOKEN_UNTIL: return "TOKEN_UNTIL";
        case TOKEN_CASE: return "TOKEN_CASE";
        case TOKEN_READ: return "TOKEN_READ";
        case TOKEN_READLN: return "TOKEN_READLN";
        case TOKEN_WRITE: return "TOKEN_WRITE";
        case TOKEN_WRITELN: return "TOKEN_WRITELN";
        case TOKEN_ARRAY: return "TOKEN_ARRAY";
        case TOKEN_OF: return "TOKEN_OF";
        case TOKEN_INTEGER: return "TOKEN_INTEGER";
        case TOKEN_REAL: return "TOKEN_REAL";
        case TOKEN_BOOLEAN: return "TOKEN_BOOLEAN";
        case TOKEN_CHAR: return "TOKEN_CHAR";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_MOD: return "TOKEN_MOD";
        case TOKEN_AND: return "TOKEN_AND";
        case TOKEN_OR: return "TOKEN_OR";
        case TOKEN_NOT: return "TOKEN_NOT";
        case TOKEN_TRUE: return "TOKEN_TRUE";
        case TOKEN_FALSE: return "TOKEN_FALSE";
        
        /* 标识符和常量 */
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_INTEGER_CONST: return "TOKEN_INTEGER_CONST";
        case TOKEN_REAL_CONST: return "TOKEN_REAL_CONST";
        case TOKEN_CHAR_CONST: return "TOKEN_CHAR_CONST";
        case TOKEN_STRING_CONST: return "TOKEN_STRING_CONST";
        
        /* 运算符 */
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MULTIPLY: return "TOKEN_MULTIPLY";
        case TOKEN_DIVIDE: return "TOKEN_DIVIDE";
        case TOKEN_EQ: return "TOKEN_EQ";
        case TOKEN_NE: return "TOKEN_NE";
        case TOKEN_LT: return "TOKEN_LT";
        case TOKEN_LE: return "TOKEN_LE";
        case TOKEN_GT: return "TOKEN_GT";
        case TOKEN_GE: return "TOKEN_GE";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        
        /* 分隔符 */
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_DOT: return "TOKEN_DOT";
        case TOKEN_DOUBLEDOT: return "TOKEN_DOUBLEDOT";
        case TOKEN_LPAREN: return "TOKEN_LPAREN";
        case TOKEN_RPAREN: return "TOKEN_RPAREN";
        case TOKEN_LBRACKET: return "TOKEN_LBRACKET";
        case TOKEN_RBRACKET: return "TOKEN_RBRACKET";
        case TOKEN_COLON: return "TOKEN_COLON";
        
        default: return "UNKNOWN_TOKEN";
    }
}

/**
 * @brief 根据字符串查找关键字Token类型（不区分大小写）
 */
TokenType lookup_keyword(const char *name) {
    if (name == NULL || *name == '\0') {
        return TOKEN_ERROR;
    }
    
    /* 确保关键字表已初始化 */
    if (!keyword_table_initialized) {
        init_keyword_table();
    }
    
    /* 遍历关键字表进行查找 */
    for (int i = 0; keyword_table[i].name != NULL; i++) {
        if (keyword_name_iequal(name, keyword_table[i].name) == 0) {
            return keyword_table[i].type;
        }
    }
    
    return TOKEN_ERROR;
}

/**
 * @brief 判断Token是否为关键字
 */
int is_keyword(TokenType type) {
    /* 关键字的TokenType值范围 */
    return (type >= TOKEN_PROGRAM && type <= TOKEN_FALSE) ||
           (type == TOKEN_DIV || type == TOKEN_MOD || 
            type == TOKEN_AND || type == TOKEN_OR || 
            type == TOKEN_NOT);
}
