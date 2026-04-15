/**
 * @file lexer.c
 * @brief 词法分析器实现
 * @author 组员1：词法分析器开发
 * 
 * 本文件实现了Pascal-S语言的词法分析器，采用手写方式实现。
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

#include "lexer.h"
#include "error.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 词法分析器全局状态 */
static LexerState g_lexer_state;
static int g_lexer_initialized = 0;

/* ========== 辅助函数声明 ========== */
static void update_position(int c);
static int read_char(void);
static void skip_whitespace_and_comments(void);
static int is_identifier_start(int c);
static int is_identifier_part(int c);
static TokenType scan_identifier(void);
static TokenType scan_number(void);
static TokenType scan_string(void);
static TokenType scan_operator_or_delimiter(void);

/* ========== 词法分析器初始化与管理 ========== */

/**
 * @brief 初始化词法分析器状态
 */
static void init_lexer_state(void) {
    memset(&g_lexer_state, 0, sizeof(LexerState));
    g_lexer_state.current_line = 1;
    g_lexer_state.current_column = 1;
    g_lexer_state.has_peek = 0;
}

/**
 * @brief 初始化词法分析器
 */
int init_lexer(const char *filename) {
    if (filename == NULL) {
        fprintf(stderr, "Error: NULL filename provided to lexer\n");
        return -1;
    }
    
    /* 初始化状态 */
    init_lexer_state();
    
    /* 打开输入文件 */
    g_lexer_state.input_file = fopen(filename, "r");
    if (g_lexer_state.input_file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    g_lexer_state.filename = filename;
    g_lexer_initialized = 1;
    
    /* 初始化关键字表 */
    init_keyword_table();
    
    /* 初始化错误处理 */
    init_error_list();
    
    return 0;
}

/**
 * @brief 关闭词法分析器
 */
void close_lexer(void) {
    if (g_lexer_state.input_file != NULL) {
        fclose(g_lexer_state.input_file);
        g_lexer_state.input_file = NULL;
    }
    g_lexer_initialized = 0;
    free_error_list();
}

/**
 * @brief 重置词法分析器状态（可用于重新分析）
 */
void reset_lexer(void) {
    if (g_lexer_state.input_file != NULL) {
        fseek(g_lexer_state.input_file, 0, SEEK_SET);
    }
    init_lexer_state();
    clear_errors();
}

/* ========== 字符输入辅助函数 ========== */

/**
 * @brief 更新行列号
 */
static void update_position(int c) {
    if (c == '\n') {
        g_lexer_state.current_line++;
        g_lexer_state.current_column = 1;
    } else if (c != EOF) {
        g_lexer_state.current_column++;
    }
}

/**
 * @brief 从输入文件读取一个字符
 * @return 读取的字符，到达文件尾返回EOF
 */
static int read_char(void) {
    int c = fgetc(g_lexer_state.input_file);
    update_position(c);
    return c;
}

/* ========== 词法分析核心函数 ========== */

/**
 * @brief 扫描字符串常量
 * @return Token类型
 */
static TokenType scan_string(void) {
    char buffer[1024];
    int pos = 0;
    int c;
    int start_line = g_lexer_state.current_line;
    int start_col = g_lexer_state.current_column - 1; /* 单引号开始的列 */
    
    /* 读取开始的单引号后的字符 */
    while ((c = read_char()) != EOF) {
        /* 检查换行 - 字符串不能跨行 */
        if (c == '\n') {
            add_error(LEX_ERROR_INVALID_CHAR_CONST, start_line, start_col,
                      "String constant cannot span multiple lines");
            g_lexer_state.current_token.value.string_val[0] = '\0';
            return TOKEN_ERROR;
        }
        
        if (c == '\'') {
            /* 检查下一个字符是否为转义单引号 */
            int next = fgetc(g_lexer_state.input_file);
            update_position(next);
            
            if (next == '\'') {
                /* 转义单引号 '' */
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = '\'';
                }
                continue;
            } else if (next == EOF) {
                add_error(LEX_ERROR_INVALID_CHAR_CONST, start_line, start_col,
                          "Unterminated string constant");
                return TOKEN_ERROR;
            } else {
                /* 字符串结束 */
                ungetc(next, g_lexer_state.input_file);
                g_lexer_state.current_column--;
                break;
            }
        }
        
        if (pos < sizeof(buffer) - 1) {
            buffer[pos++] = (char)c;
        }
    }
    
    buffer[pos] = '\0';
    
    if (c == EOF) {
        add_error(LEX_ERROR_INVALID_CHAR_CONST, start_line, start_col,
                  "Unterminated string constant");
        return TOKEN_ERROR;
    }
    
    /* 如果字符串只包含一个字符且长度为1，可能是字符常量 */
    if (pos == 1) {
        g_lexer_state.current_token.value.char_val = buffer[0];
        return TOKEN_CHAR_CONST;
    }
    
    strncpy(g_lexer_state.current_token.value.string_val, buffer, sizeof(buffer) - 1);
    g_lexer_state.current_token.value.string_val[sizeof(buffer) - 1] = '\0';
    
    return TOKEN_STRING_CONST;
}

/**
 * @brief 跳过空白字符和注释
 */
static void skip_whitespace_and_comments(void) {
    int c;
    
    while ((c = read_char()) != EOF) {
        /* 跳过空白字符 */
        if (isspace(c)) {
            continue;
        }
        
        /* 跳过单行注释 { ... } */
        if (c == '{') {
            int comment_line = g_lexer_state.current_line;
            int comment_col = g_lexer_state.current_column - 1;
            
            while ((c = read_char()) != EOF) {
                if (c == '}') {
                    break;
                }
            }
            
            if (c == EOF) {
                add_error(LEX_ERROR_UNTERMINATED_COMMENT, comment_line, 
                          comment_col, "Unterminated comment");
                return;
            }
            continue;
        }
        
        /* 跳过Pascal标准注释 (* ... *) */
        if (c == '(') {
            int next = fgetc(g_lexer_state.input_file);
            update_position(next);
            
            if (next == '*') {
                /* 这是一个 (* ... *) 注释 */
                int comment_line = g_lexer_state.current_line;
                int comment_col = g_lexer_state.current_column - 2;
                
                while ((c = read_char()) != EOF) {
                    if (c == '*') {
                        int after = fgetc(g_lexer_state.input_file);
                        update_position(after);
                        if (after == ')') {
                            break;
                        } else if (after != EOF) {
                            ungetc(after, g_lexer_state.input_file);
                            g_lexer_state.current_column--;
                        }
                    }
                }
                
                if (c == EOF) {
                    add_error(LEX_ERROR_UNTERMINATED_COMMENT, comment_line,
                              comment_col, "Unterminated comment");
                    return;
                }
                continue;
            } else {
                /* '(' 不是注释的开始，放回并返回 */
                if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                ungetc(c, g_lexer_state.input_file);
                g_lexer_state.current_column--;
                return;
            }
        }
        
        /* 跳过C风格注释 slash-star ... star-slash */
        if (c == '/') {
            int next = fgetc(g_lexer_state.input_file);
            update_position(next);
            
            if (next == '*') {
                /* 这是一个 slash-star ... star-slash 注释 */
                int comment_line = g_lexer_state.current_line;
                int comment_col = g_lexer_state.current_column - 2;
                
                while ((c = read_char()) != EOF) {
                    if (c == '*') {
                        int after = fgetc(g_lexer_state.input_file);
                        update_position(after);
                        if (after == '/') {
                            break;
                        } else if (after != EOF) {
                            ungetc(after, g_lexer_state.input_file);
                            g_lexer_state.current_column--;
                        }
                    }
                }
                
                if (c == EOF) {
                    add_error(LEX_ERROR_UNTERMINATED_COMMENT, comment_line,
                              comment_col, "Unterminated comment");
                    return;
                }
                continue;
            } else {
                /* '/' 不是注释的开始，放回并返回 */
                if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                ungetc(c, g_lexer_state.input_file);
                g_lexer_state.current_column--;
                return;
            }
        }
        
        /* 非空白字符，停止跳过 */
        ungetc(c, g_lexer_state.input_file);
        g_lexer_state.current_column--;
        return;
    }
}

/**
 * @brief 判断字符是否可以作为标识符的首字符
 */
static int is_identifier_start(int c) {
    return isalpha(c) || c == '_';
}

/**
 * @brief 判断字符是否可以作为标识符的后续字符
 */
static int is_identifier_part(int c) {
    return isalnum(c) || c == '_';
}

/**
 * @brief 扫描标识符或关键字
 * @return Token类型
 */
static TokenType scan_identifier(void) {
    char buffer[256];
    int pos = 0;
    int c;
    
    while ((c = read_char()) != EOF && is_identifier_part(c)) {
        if (pos < sizeof(buffer) - 1) {
            buffer[pos++] = (char)c;
        }
    }
    buffer[pos] = '\0';
    
    /* 将多读的字符放回 */
    if (c != EOF) {
        ungetc(c, g_lexer_state.input_file);
        g_lexer_state.current_column--;
    }
    
    /* 查找关键字 */
    TokenType type = lookup_keyword(buffer);
    if (type != TOKEN_ERROR) {
        return type;
    }
    
    /* 普通标识符 */
    if (pos > 255) {
        add_error(LEX_ERROR_STRING_TOO_LONG, g_lexer_state.current_line,
                  g_lexer_state.current_column - pos,
                  "Identifier too long (max 255 characters)");
        pos = 255;
        buffer[pos] = '\0';
    }
    
    strncpy(g_lexer_state.current_token.value.str_val, buffer, 255);
    g_lexer_state.current_token.value.str_val[255] = '\0';
    
    return TOKEN_IDENTIFIER;
}

/**
 * @brief 扫描数字常量（整数或实数）
 * @return Token类型
 */
static TokenType scan_number(void) {
    char buffer[256];
    int pos = 0;
    int c;
    int has_dot = 0;
    int has_exp = 0;
    
    while ((c = read_char()) != EOF) {
        if (!isdigit(c) && c != '.' && c != 'e' && c != 'E' && 
            c != '+' && c != '-') {
            /* 不是数字字符 */
            ungetc(c, g_lexer_state.input_file);
            g_lexer_state.current_column--;
            break;
        }
        
        /* 检查小数点 */
        if (c == '.') {
            int next = fgetc(g_lexer_state.input_file);
            update_position(next);
            
            if (isdigit(next)) {
                /* 实数 */
                has_dot = 1;
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = '.';
                }
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = (char)next;
                }
                
                /* 读取小数部分的其余数字 */
                while ((c = read_char()) != EOF && isdigit(c)) {
                    if (pos < sizeof(buffer) - 1) {
                        buffer[pos++] = (char)c;
                    }
                }
                
                if (c != EOF) {
                    ungetc(c, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
            } else if (next == '.') {
                /* 可能是 .. 分隔符 */
                ungetc(next, g_lexer_state.input_file);
                g_lexer_state.current_column--;
                buffer[pos] = '\0';
                g_lexer_state.current_token.value.int_val = atoi(buffer);
                return TOKEN_INTEGER_CONST;
            } else {
                /* 单独的点是范围运算符，放回 */
                if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                buffer[pos] = '\0';
                g_lexer_state.current_token.value.int_val = atoi(buffer);
                return TOKEN_INTEGER_CONST;
            }
            continue;
        }
        
        /* 检查指数部分 */
        if (c == 'e' || c == 'E') {
            has_exp = 1;
            if (pos < sizeof(buffer) - 1) {
                buffer[pos++] = (char)c;
            }
            
            /* 指数符号 */
            c = read_char();
            if (c == '+' || c == '-') {
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = (char)c;
                }
            } else if (isdigit(c)) {
                /* 指数没有符号，直接是数字 */
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = (char)c;
                }
            } else {
                /* 格式错误 */
                if (c != EOF) {
                    ungetc(c, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                add_error(LEX_ERROR_INVALID_NUMBER, g_lexer_state.current_line,
                          g_lexer_state.current_column,
                          "Invalid number format");
                return TOKEN_ERROR;
            }
            
            /* 读取指数部分的其余数字 */
            while ((c = read_char()) != EOF && isdigit(c)) {
                if (pos < sizeof(buffer) - 1) {
                    buffer[pos++] = (char)c;
                }
            }
            
            if (c != EOF) {
                ungetc(c, g_lexer_state.input_file);
                g_lexer_state.current_column--;
            }
            continue;
        }
        
        if (pos < sizeof(buffer) - 1) {
            buffer[pos++] = (char)c;
        }
    }
    
    buffer[pos] = '\0';
    
    /* 返回对应的Token类型 */
    if (has_dot || has_exp) {
        g_lexer_state.current_token.value.real_val = atof(buffer);
        return TOKEN_REAL_CONST;
    } else {
        g_lexer_state.current_token.value.int_val = atoi(buffer);
        return TOKEN_INTEGER_CONST;
    }
}

/**
 * @brief 扫描运算符或分隔符
 * @return Token类型
 */
static TokenType scan_operator_or_delimiter(void) {
    int c = read_char();
    
    switch (c) {
        /* 单字符运算符和分隔符 */
        case '+': return TOKEN_PLUS;
        case '-': return TOKEN_MINUS;
        case '*': return TOKEN_MULTIPLY;
        
        case '=': return TOKEN_EQ;
        case ',': return TOKEN_COMMA;
        case ';': return TOKEN_SEMICOLON;
        case '(': return TOKEN_LPAREN;
        case ')': return TOKEN_RPAREN;
        case '[': return TOKEN_LBRACKET;
        case ']': return TOKEN_RBRACKET;
        
        case '.':
            /* 检查是否为 .. */
            {
                int next = fgetc(g_lexer_state.input_file);
                update_position(next);
                if (next == '.') {
                    return TOKEN_DOUBLEDOT;
                } else if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                return TOKEN_DOT;
            }
        
        case ':':
            /* 检查是否为 := 赋值运算符 */
            {
                int next = fgetc(g_lexer_state.input_file);
                update_position(next);
                if (next == '=') {
                    return TOKEN_ASSIGN;
                } else if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                return TOKEN_COLON;
            }
        
        case '<':
            /* 检查是否为 <> 或 <= */
            {
                int next = fgetc(g_lexer_state.input_file);
                update_position(next);
                if (next == '>') {
                    return TOKEN_NE;
                } else if (next == '=') {
                    return TOKEN_LE;
                } else if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                return TOKEN_LT;
            }
        
        case '>':
            /* 检查是否为 >= */
            {
                int next = fgetc(g_lexer_state.input_file);
                update_position(next);
                if (next == '=') {
                    return TOKEN_GE;
                } else if (next != EOF) {
                    ungetc(next, g_lexer_state.input_file);
                    g_lexer_state.current_column--;
                }
                return TOKEN_GT;
            }
        
        case '/':
            /* 除法运算符 */
            return TOKEN_DIVIDE;
        
        case '\'':
            /* 字符常量或字符串常量 */
            return scan_string();
        
        case EOF:
            return TOKEN_EOF;
        
        default:
            /* 非法字符 */
            add_error(LEX_ERROR_ILLEGAL_CHAR, g_lexer_state.current_line,
                      g_lexer_state.current_column - 1,
                      "Illegal character: '%c' (0x%02X)", c, c);
            return TOKEN_ERROR;
    }
}

/* ========== 主词法分析函数 ========== */

/**
 * @brief 获取下一个Token（消费式）
 * @param token 用于存储Token信息的结构指针
 * @return 成功返回Token类型，失败返回TOKEN_ERROR
 */
TokenType get_next_token(Token *token) {
    TokenType type;
    
    /* 检查是否已初始化 */
    if (!g_lexer_initialized) {
        fprintf(stderr, "Error: Lexer not initialized\n");
        return TOKEN_ERROR;
    }
    
    /* 如果有预读的Token，直接返回 */
    if (g_lexer_state.has_peek) {
        g_lexer_state.has_peek = 0;
        *token = g_lexer_state.peeked_token;
        return token->type;
    }
    
    /* 跳过空白字符和注释 */
    skip_whitespace_and_comments();
    
    /* 设置Token的初始位置 */
    g_lexer_state.current_token.line = g_lexer_state.current_line;
    g_lexer_state.current_token.column = g_lexer_state.current_column;
    
    /* 读取第一个字符 */
    int c = read_char();
    
    /* 处理文件结束 */
    if (c == EOF) {
        g_lexer_state.current_token.type = TOKEN_EOF;
        *token = g_lexer_state.current_token;
        return TOKEN_EOF;
    }
    
    /* 根据首字符确定Token类型 */
    if (is_identifier_start(c)) {
        /* 将字符放回，因为scan_identifier会重新读取 */
        ungetc(c, g_lexer_state.input_file);
        g_lexer_state.current_column--;
        type = scan_identifier();
    } else if (isdigit(c)) {
        /* 将数字放回，因为scan_number会重新读取 */
        ungetc(c, g_lexer_state.input_file);
        g_lexer_state.current_column--;
        type = scan_number();
    } else {
        /* 运算符或分隔符 */
        ungetc(c, g_lexer_state.input_file);
        g_lexer_state.current_column--;
        type = scan_operator_or_delimiter();
    }
    
    /* 设置Token类型 */
    g_lexer_state.current_token.type = type;
    
    /* 复制Token */
    *token = g_lexer_state.current_token;
    
    return type;
}

/* ========== Token获取核心接口实现 ========== */

/**
 * @brief 获取当前Token（用于断言或跳过检查）
 */
TokenType current_token(Token *token) {
    if (token != NULL) {
        *token = g_lexer_state.current_token;
    }
    return g_lexer_state.current_token.type;
}

/**
 * @brief 跳过当前Token（消费但不处理）
 */
TokenType skip_token(void) {
    return get_next_token(&g_lexer_state.current_token);
}

/* ========== Token匹配接口实现 ========== */

/**
 * @brief 检查当前Token是否为指定类型
 */
int check_token(TokenType type) {
    return g_lexer_state.current_token.type == type;
}

/**
 * @brief 匹配指定类型的Token（不消费）
 */
int match_token(TokenType expected) {
    return g_lexer_state.current_token.type == expected;
}

/**
 * @brief 匹配指定类型的Token并消费
 */
int expect_token(TokenType expected) {
    if (g_lexer_state.current_token.type == expected) {
        get_next_token(&g_lexer_state.current_token);
        return 1;
    }
    return 0;
}

/* ========== Token值获取便捷函数实现 ========== */

/**
 * @brief 获取当前Token的整数值
 */
int token_int_value(void) {
    return g_lexer_state.current_token.value.int_val;
}

/**
 * @brief 获取当前Token的实数值
 */
double token_real_value(void) {
    return g_lexer_state.current_token.value.real_val;
}

/**
 * @brief 获取当前Token的字符串值（标识符或字符串常量）
 */
const char* token_string_value(void) {
    TokenType type = g_lexer_state.current_token.type;
    if (type == TOKEN_IDENTIFIER) {
        return g_lexer_state.current_token.value.str_val;
    } else if (type == TOKEN_STRING_CONST) {
        return g_lexer_state.current_token.value.string_val;
    }
    return "";
}

/* ========== 位置信息查询接口实现 ========== */

/**
 * @brief 获取当前行号
 */
int get_current_line(void) {
    return g_lexer_state.current_line;
}

/**
 * @brief 获取当前列号
 */
int get_current_column(void) {
    return g_lexer_state.current_column;
}

/**
 * @brief 获取当前文件名
 */
const char* get_current_filename(void) {
    return g_lexer_state.filename;
}

/**
 * @brief 获取词法分析器完整状态（只读）
 */
const LexerState* get_lexer_state(void) {
    return &g_lexer_state;
}

/* ========== 错误处理接口实现 ========== */

/**
 * @brief 检查是否有致命错误（导致编译停止的错误）
 */
int lexer_has_fatal_errors(void) {
    return has_errors();
}

/* ========== 调试和测试接口实现 ========== */

/**
 * @brief 打印Token信息（用于调试）
 */
void print_token(const Token *token) {
    print_token_to(token, stdout);
}

/**
 * @brief 打印Token信息到指定文件
 */
void print_token_to(const Token *token, FILE *output) {
    if (token == NULL) {
        fprintf(output, "Token: NULL\n");
        return;
    }
    
    fprintf(output, "Token: %-20s Line: %-4d Column: %-4d",
           get_token_display_name(token->type),
           token->line, token->column);
    
    /* 根据类型打印额外信息 */
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            fprintf(output, " Value: '%s'", token->value.str_val);
            break;
        case TOKEN_INTEGER_CONST:
            fprintf(output, " Value: %d", token->value.int_val);
            break;
        case TOKEN_REAL_CONST:
            fprintf(output, " Value: %f", token->value.real_val);
            break;
        case TOKEN_CHAR_CONST:
            fprintf(output, " Value: '%c'", token->value.char_val);
            break;
        case TOKEN_STRING_CONST:
            fprintf(output, " Value: \"%s\"", token->value.string_val);
            break;
        default:
            break;
    }
    
    fprintf(output, "\n");
}

/**
 * @brief 获取Token类型的用户友好名称
 */
const char* get_token_display_name(TokenType type) {
    return token_type_to_string(type);
}

/**
 * @brief 词法分析器测试函数
 */
void test_lexer(void) {
    Token token;
    TokenType type;
    
    printf("=== Lexer Test Results ===\n\n");
    
    while ((type = get_next_token(&token)) != TOKEN_EOF) {
        print_token(&token);
        
        if (type == TOKEN_ERROR) {
            printf("Warning: Encountered error token\n");
        }
    }
    
    /* 打印文件结束Token */
    printf("\nToken: %-20s Line: %-4d Column: %-4d\n",
           get_token_display_name(TOKEN_EOF),
           token.line, token.column);
    
    printf("\n=== Lexer Test Complete ===\n");
    
    /* 检查是否有错误 */
    if (has_errors()) {
        printf("\nLexer encountered %d error(s)\n", get_error_count());
    }
}
