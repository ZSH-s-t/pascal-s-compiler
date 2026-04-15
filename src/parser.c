/**
 * @file parser.c
 * @brief Pascal-S语法分析器（递归下降）
 *
 * 文法参照Pascal-S定义，实现了：
 * - 程序结构 program ... .
 * - 常量/变量/子程序声明
 * - 语句（赋值、if、for、复合语句、read/write）
 * - 表达式（含优先级）
 * 错误处理：报错后尝试同步到分号或end，继续分析。
 */

#include "parser.h"
#include "lexer.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 当前token与预读管理 */
static Token cur_token;
static Token lookahead;
static int lookahead_valid = 0;

/* 获取下一个token */
static void advance(void) {
    if (lookahead_valid) {
        cur_token = lookahead;
        lookahead_valid = 0;
    } else {
        get_next_token(&cur_token);
    }
}

/* 预读一个token（不消费） */
static TokenType peek(void) {
    if (!lookahead_valid) {
        get_next_token(&lookahead);
        lookahead_valid = 1;
    }
    return lookahead.type;
}

/* 错误恢复：跳过token直到遇到同步集合中的类型 */
static void sync_to(TokenType sync_set[], int n) {
    while (1) {
        TokenType t = peek();
        if (t == TOKEN_EOF) break;
        for (int i = 0; i < n; i++) {
            if (t == sync_set[i]) return;
        }
        advance();
    }
}

/* 匹配期望的token，失败则报错并尝试恢复 */
static int expect(TokenType expected) {
    if (cur_token.type == expected) {
        advance();
        return 1;
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected '%s', got '%s'",
                 get_token_display_name(expected),
                 get_token_display_name(cur_token.type));
        syntax_error(cur_token.line, msg);
        /* 简单恢复：跳过当前token */
        advance();
        return 0;
    }
}

/* ---------- 前向声明 ---------- */
static ASTNode *parse_const_declarations(void);
static ASTNode *parse_var_declarations(void);
static ASTNode *parse_subprogram_declarations(void);
static ASTNode *parse_compound_statement(void);
static ASTNode *parse_statement_list(void);
static ASTNode *parse_statement(void);
static ASTNode *parse_expression(void);
static ASTNode *parse_simple_expression(void);
static ASTNode *parse_term(void);
static ASTNode *parse_factor(void);
static ASTNode *parse_variable(void);
static ASTNode *parse_id_list(void);
static ASTNode *parse_expression_list(void);
static ASTNode *parse_const_value(void);
static ASTNode *parse_period(void);

/* ---------- 程序入口 ---------- */
ASTNode *parse_program(void) {
    advance();  /* 读取第一个token */
    if (cur_token.type != TOKEN_PROGRAM) {
        syntax_error(cur_token.line, "Program must start with 'program'");
        return NULL;
    }
    int line = cur_token.line;
    advance();
    if (cur_token.type != TOKEN_IDENTIFIER) {
        syntax_error(cur_token.line, "Expected program name");
        return NULL;
    }
    ASTNode *prog = ast_new_node(AST_PROGRAM, line);
    strcpy(prog->data.program.prog_name, cur_token.value.str_val);
    advance();

    /* 可选的 (idlist) */
    if (cur_token.type == TOKEN_LPAREN) {
        advance();  /* cur_token = ( */
        if (peek() == TOKEN_IDENTIFIER) advance();  /* cur_token = 第一个参数名 */
        parse_id_list();
        expect(TOKEN_RPAREN);
    }
    expect(TOKEN_SEMICOLON);

    /* 解析各部分 */
    prog->data.program.const_decls = parse_const_declarations();
    prog->data.program.var_decls = parse_var_declarations();
    prog->data.program.subprog_decls = parse_subprogram_declarations();
    prog->data.program.body = parse_compound_statement();
    expect(TOKEN_DOT);

    /* 检查是否有多余token */
    if (peek() != TOKEN_EOF) {
        syntax_error(cur_token.line, "Extra tokens after end of program");
    }
    return prog;
}

/* ---------- 常量声明 ---------- */
static ASTNode *parse_const_declarations(void) {
    if (cur_token.type != TOKEN_CONST) return NULL;
    advance();  /* cur_token = 第一个标识符 */
    ASTNode *head = NULL, *tail = NULL;
    while (cur_token.type == TOKEN_IDENTIFIER) {
        int line = cur_token.line;
        ASTNode *decl = ast_new_node(AST_CONST_DECL, line);
        strcpy(decl->data.const_decl.name, cur_token.value.str_val);
        advance();
        expect(TOKEN_EQ);
        decl->data.const_decl.value = parse_const_value();
        if (!head) head = decl;
        else tail->next = decl;
        tail = decl;
        if (cur_token.type == TOKEN_SEMICOLON) advance();
        else break;
    }
    return head;
}

/* 常量值解析 */
static ASTNode *parse_const_value(void) {
    int line = cur_token.line;
    switch (cur_token.type) {
        case TOKEN_PLUS:
            advance();
            if (cur_token.type == TOKEN_INTEGER_CONST)
                return ast_new_const_int(cur_token.value.int_val, line);
            else if (cur_token.type == TOKEN_REAL_CONST)
                return ast_new_const_real(cur_token.value.real_val, line);
            break;
        case TOKEN_MINUS:
            advance();
            if (cur_token.type == TOKEN_INTEGER_CONST)
                return ast_new_const_int(-cur_token.value.int_val, line);
            else if (cur_token.type == TOKEN_REAL_CONST)
                return ast_new_const_real(-cur_token.value.real_val, line);
            break;
        case TOKEN_INTEGER_CONST:
            advance();
            return ast_new_const_int(cur_token.value.int_val, line);
        case TOKEN_REAL_CONST:
            advance();
            return ast_new_const_real(cur_token.value.real_val, line);
        case TOKEN_CHAR_CONST:
            advance();
            return ast_new_const_char(cur_token.value.char_val, line);
        default:
            syntax_error(line, "Invalid constant value");
            advance();
            return ast_new_const_int(0, line);
    }
    advance();
    return ast_new_const_int(0, line);
}

/* ---------- 变量声明 ---------- */
static ASTNode *parse_var_declarations(void) {
    if (cur_token.type != TOKEN_VAR) return NULL;
    advance();  /* cur_token = 第一个标识符 */
    ASTNode *head = NULL, *tail = NULL;
    while (cur_token.type == TOKEN_IDENTIFIER) {
        int line = cur_token.line;
        ASTNode *decl = ast_new_node(AST_VAR_DECL, line);
        decl->data.var_decl.id_list = parse_id_list();
        expect(TOKEN_COLON);

        /* 解析类型 */
        if (cur_token.type == TOKEN_ARRAY) {
            advance();
            expect(TOKEN_LBRACKET);
            decl->data.var_decl.array_bounds = parse_period();
            expect(TOKEN_RBRACKET);
            expect(TOKEN_OF);
            decl->data.var_decl.data_type = TYPE_ARRAY;
        } else {
            decl->data.var_decl.array_bounds = NULL;
        }
        if (decl->data.var_decl.data_type != TYPE_ARRAY) {
            if (cur_token.type == TOKEN_INTEGER)
                decl->data.var_decl.data_type = TYPE_INTEGER;
            else if (cur_token.type == TOKEN_REAL)
                decl->data.var_decl.data_type = TYPE_REAL;
            else if (cur_token.type == TOKEN_BOOLEAN)
                decl->data.var_decl.data_type = TYPE_BOOLEAN;
            else if (cur_token.type == TOKEN_CHAR)
                decl->data.var_decl.data_type = TYPE_CHAR;
            else {
                syntax_error(cur_token.line, "Expected type");
                decl->data.var_decl.data_type = TYPE_INTEGER;
            }
            advance();
        } else {
            /* 数组元素类型 */
            if (cur_token.type == TOKEN_INTEGER)
                decl->data.var_decl.data_type = TYPE_INTEGER;
            else if (cur_token.type == TOKEN_REAL)
                decl->data.var_decl.data_type = TYPE_REAL;
            else if (cur_token.type == TOKEN_BOOLEAN)
                decl->data.var_decl.data_type = TYPE_BOOLEAN;
            else if (cur_token.type == TOKEN_CHAR)
                decl->data.var_decl.data_type = TYPE_CHAR;
            advance();
        }

        if (!head) head = decl;
        else tail->next = decl;
        tail = decl;
        expect(TOKEN_SEMICOLON);
        /* cur_token 现在是分号后的 token，检查是否是下一个变量声明 */
        if (cur_token.type != TOKEN_IDENTIFIER) break;
    }
    return head;
}

/* 下标范围（简化，返回表达式列表） */
static ASTNode *parse_period(void) {
    /* 返回一个表示范围的表达式链表 */
    ASTNode *first = parse_expression();
    expect(TOKEN_DOUBLEDOT);
    ASTNode *second = parse_expression();
    ASTNode *list = ast_new_node(AST_STMT_LIST, first->line);
    list->data.stmt_list.first = first;
    list->data.stmt_list.last = second;
    first->next = second;
    return list;
}

/* 标识符列表 */
static ASTNode *parse_id_list(void) {
    ASTNode *head = NULL;
    do {
        if (cur_token.type != TOKEN_IDENTIFIER) break;
        head = ast_append_id(head, cur_token.value.str_val, cur_token.line);
        advance();
        if (cur_token.type == TOKEN_COMMA) advance();
        else break;
    } while (1);
    return head;
}

/* ---------- 子程序声明 ---------- */
static ASTNode *parse_subprogram_declarations(void) {
    ASTNode *head = NULL, *tail = NULL;
    while (cur_token.type == TOKEN_PROCEDURE || cur_token.type == TOKEN_FUNCTION) {
        int is_func = (cur_token.type == TOKEN_FUNCTION);
        int line = cur_token.line;
        ASTNode *sub = ast_new_node(AST_SUBPROG_DECL, line);
        sub->data.subprog.is_function = is_func;
        advance();
        if (cur_token.type != TOKEN_IDENTIFIER) {
            syntax_error(cur_token.line, "Expected subprogram name");
            return head;
        }
        strcpy(sub->data.subprog.name, cur_token.value.str_val);
        advance();

        /* 参数列表 */
        if (cur_token.type == TOKEN_LPAREN) {
            advance();  /* cur_token = ( */
            /* 简化：解析参数列表 */
            ASTNode *params = NULL;
            while (cur_token.type != TOKEN_RPAREN) {
                int is_var = 0;
                if (cur_token.type == TOKEN_VAR) {
                    advance(); is_var = 1;
                }
                ASTNode *ids = parse_id_list();
                expect(TOKEN_COLON);
                DataType dt = TYPE_INTEGER;
                if (cur_token.type == TOKEN_INTEGER) dt = TYPE_INTEGER;
                else if (cur_token.type == TOKEN_REAL) dt = TYPE_REAL;
                else if (cur_token.type == TOKEN_BOOLEAN) dt = TYPE_BOOLEAN;
                else if (cur_token.type == TOKEN_CHAR) dt = TYPE_CHAR;
                advance();
                params = ast_append_param(params, is_var, ids, dt, line);
                if (cur_token.type == TOKEN_SEMICOLON) advance();
                else break;
            }
            sub->data.subprog.params = params;
            advance();  /* 跳过 ) */
        }
        if (is_func) {
            expect(TOKEN_COLON);
            if (cur_token.type == TOKEN_INTEGER) sub->data.subprog.return_type = TYPE_INTEGER;
            else if (cur_token.type == TOKEN_REAL) sub->data.subprog.return_type = TYPE_REAL;
            else if (cur_token.type == TOKEN_BOOLEAN) sub->data.subprog.return_type = TYPE_BOOLEAN;
            else if (cur_token.type == TOKEN_CHAR) sub->data.subprog.return_type = TYPE_CHAR;
            advance();
        }
        expect(TOKEN_SEMICOLON);
        /* 子程序体（忽略常量/变量声明，直接解析复合语句） */
        sub->data.subprog.body = parse_compound_statement();
        expect(TOKEN_SEMICOLON);

        if (!head) head = sub;
        else tail->next = sub;
        tail = sub;
    }
    return head;
}

/* ---------- 复合语句 ---------- */
static ASTNode *parse_compound_statement(void) {
    if (cur_token.type != TOKEN_BEGIN) {
        syntax_error(cur_token.line, "Expected 'begin'");
        return NULL;
    }
    ASTNode *node = ast_new_node(AST_COMPOUND_STMT, cur_token.line);
    advance();  /* cur_token = begin 后的第一个token */
    node->data.compound.stmt_list = parse_statement_list();
    expect(TOKEN_END);
    return node;
}

/* 语句列表 */
static ASTNode *parse_statement_list(void) {
    ASTNode *list = ast_new_node(AST_STMT_LIST, cur_token.line);
    while (cur_token.type != TOKEN_END && cur_token.type != TOKEN_ELSE && cur_token.type != TOKEN_EOF) {
        ASTNode *stmt = parse_statement();
        if (stmt) {
            list = ast_append_stmt(list, stmt);
        }
        if (cur_token.type == TOKEN_SEMICOLON)
            advance();
        else if (cur_token.type != TOKEN_END && cur_token.type != TOKEN_ELSE) {
            /* 允许没有分号的情况（如 end 前的最后一个语句） */
        }
    }
    return list;
}

/* 单个语句 */
static ASTNode *parse_statement(void) {
    switch (cur_token.type) {
        case TOKEN_IDENTIFIER: {
            /* cur_token 已经是标识符 */
            char name[64];
            strcpy(name, cur_token.value.str_val);
            int line = cur_token.line;
            advance(); /* 移到标识符后的 token */

            if (cur_token.type == TOKEN_ASSIGN) {
                /* 赋值语句 */
                advance();  /* 移过 := */
                ASTNode *node = ast_new_node(AST_ASSIGN_STMT, line);
                node->data.assign.lhs = ast_new_var_ref(name, NULL, line);
                node->data.assign.rhs = parse_expression();
                return node;
            } else if (cur_token.type == TOKEN_LBRACKET) {
                /* 数组元素赋值 */
                advance();
                ASTNode *index = parse_expression();
                expect(TOKEN_RBRACKET);
                expect(TOKEN_ASSIGN);
                ASTNode *node = ast_new_node(AST_ASSIGN_STMT, line);
                node->data.assign.lhs = ast_new_var_ref(name, index, line);
                node->data.assign.rhs = parse_expression();
                return node;
            } else {
                /* 过程调用（或无参数函数调用）*/
                ASTNode *call = ast_new_node(AST_CALL_STMT, line);
                /* 暂不处理参数，直接返回调用节点 */
                if (cur_token.type == TOKEN_LPAREN) {
                    advance();
                    parse_expression_list();
                    expect(TOKEN_RPAREN);
                }
                return call;
            }
        }
        case TOKEN_BEGIN:
            return parse_compound_statement();
        case TOKEN_IF: {
            advance();  /* cur_token = if 后的 token */
            int line = cur_token.line;
            ASTNode *node = ast_new_node(AST_IF_STMT, line);
            node->data.if_stmt.cond = parse_expression();
            expect(TOKEN_THEN);
            node->data.if_stmt.then_part = parse_statement();
            if (cur_token.type == TOKEN_ELSE) {
                advance();  /* cur_token = else 后的 token */
                node->data.if_stmt.else_part = parse_statement();
            } else {
                node->data.if_stmt.else_part = NULL;
            }
            return node;
        }
        case TOKEN_FOR: {
            advance();  /* cur_token = for 后的 token */
            int line = cur_token.line;
            ASTNode *node = ast_new_node(AST_FOR_STMT, line);
            if (cur_token.type != TOKEN_IDENTIFIER) {
                syntax_error(line, "Expected loop variable");
                return NULL;
            }
            strcpy(node->data.for_stmt.var_name, cur_token.value.str_val);
            advance();
            expect(TOKEN_ASSIGN);
            node->data.for_stmt.start_expr = parse_expression();
            expect(TOKEN_TO);
            node->data.for_stmt.end_expr = parse_expression();
            expect(TOKEN_DO);
            node->data.for_stmt.body = parse_statement();
            return node;
        }
        case TOKEN_READ:
            advance();
            expect(TOKEN_LPAREN);
            parse_variable(); /* 变量列表简化 */
            while (cur_token.type == TOKEN_COMMA) {
                advance();
                parse_variable();
            }
            expect(TOKEN_RPAREN);
            return ast_new_node(AST_READ_STMT, cur_token.line);
        case TOKEN_WRITE:
            advance();
            expect(TOKEN_LPAREN);
            parse_expression_list();
            expect(TOKEN_RPAREN);
            return ast_new_node(AST_WRITE_STMT, cur_token.line);
        default:
            syntax_error(cur_token.line, "Unexpected token in statement");
            advance();
            return NULL;
    }
}

/* 表达式列表 */
static ASTNode *parse_expression_list(void) {
    ASTNode *list = ast_new_node(AST_STMT_LIST, cur_token.line);
    ASTNode *first = parse_expression();
    list->data.stmt_list.first = list->data.stmt_list.last = first;
    while (cur_token.type == TOKEN_COMMA) {
        advance();
        ASTNode *expr = parse_expression();
        list->data.stmt_list.last->next = expr;
        list->data.stmt_list.last = expr;
    }
    return list;
}

/* 变量解析（用于read语句等） */
static ASTNode *parse_variable(void) {
    if (cur_token.type != TOKEN_IDENTIFIER) {
        syntax_error(cur_token.line, "Expected variable");
        return NULL;
    }
    char name[64];
    strcpy(name, cur_token.value.str_val);
    int line = cur_token.line;
    advance();
    ASTNode *index = NULL;
    if (cur_token.type == TOKEN_LBRACKET) {
        advance();
        index = parse_expression();
        expect(TOKEN_RBRACKET);
    }
    return ast_new_var_ref(name, index, line);
}

/* ---------- 表达式（含关系运算符）---------- */
static ASTNode *parse_expression(void) {
    ASTNode *left = parse_simple_expression();
    TokenType relop = cur_token.type;
    if (relop == TOKEN_EQ || relop == TOKEN_NE || relop == TOKEN_LT ||
        relop == TOKEN_LE || relop == TOKEN_GT || relop == TOKEN_GE) {
        int line = cur_token.line;
        advance();
        ASTNode *right = parse_simple_expression();
        BinaryOp op;
        switch (relop) {
            case TOKEN_EQ: op = OP_EQ; break;
            case TOKEN_NE: op = OP_NE; break;
            case TOKEN_LT: op = OP_LT; break;
            case TOKEN_LE: op = OP_LE; break;
            case TOKEN_GT: op = OP_GT; break;
            case TOKEN_GE: op = OP_GE; break;
            default: op = OP_EQ;
        }
        left = ast_new_binary_expr(op, left, right, line);
    }
    return left;
}

static ASTNode *parse_simple_expression(void) {
    ASTNode *left = parse_term();
    while (cur_token.type == TOKEN_PLUS || cur_token.type == TOKEN_MINUS ||
           cur_token.type == TOKEN_OR) {
        BinaryOp op;
        if (cur_token.type == TOKEN_PLUS) op = OP_ADD;
        else if (cur_token.type == TOKEN_MINUS) op = OP_SUB;
        else op = OP_OR;
        int line = cur_token.line;
        advance();
        ASTNode *right = parse_term();
        left = ast_new_binary_expr(op, left, right, line);
    }
    return left;
}

static ASTNode *parse_term(void) {
    ASTNode *left = parse_factor();
    while (cur_token.type == TOKEN_MULTIPLY || cur_token.type == TOKEN_DIVIDE ||
           cur_token.type == TOKEN_DIV || cur_token.type == TOKEN_MOD ||
           cur_token.type == TOKEN_AND) {
        BinaryOp op;
        if (cur_token.type == TOKEN_MULTIPLY) op = OP_MUL;
        else if (cur_token.type == TOKEN_DIVIDE) op = OP_DIV;
        else if (cur_token.type == TOKEN_DIV) op = OP_DIV_INT;
        else if (cur_token.type == TOKEN_MOD) op = OP_MOD;
        else op = OP_AND;
        int line = cur_token.line;
        advance();
        ASTNode *right = parse_factor();
        left = ast_new_binary_expr(op, left, right, line);
    }
    return left;
}

static ASTNode *parse_factor(void) {
    int line = cur_token.line;
    switch (cur_token.type) {
        case TOKEN_IDENTIFIER: {
            char name[64];
            strcpy(name, cur_token.value.str_val);
            advance();
            if (cur_token.type == TOKEN_LPAREN) {
                /* 函数调用 */
                advance();
                ASTNode *args = NULL;
                if (peek() != TOKEN_RPAREN) args = parse_expression_list();
                expect(TOKEN_RPAREN);
                /* 暂简化：函数调用视为变量引用 */
                return ast_new_var_ref(name, NULL, line);
            } else if (cur_token.type == TOKEN_LBRACKET) {
                advance();
                ASTNode *index = parse_expression();
                expect(TOKEN_RBRACKET);
                return ast_new_var_ref(name, index, line);
            } else {
                return ast_new_var_ref(name, NULL, line);
            }
        }
        case TOKEN_INTEGER_CONST:
            advance();
            return ast_new_const_int(cur_token.value.int_val, line);
        case TOKEN_REAL_CONST:
            advance();
            return ast_new_const_real(cur_token.value.real_val, line);
        case TOKEN_CHAR_CONST:
            advance();
            return ast_new_const_char(cur_token.value.char_val, line);
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            advance();
            return ast_new_const_bool(cur_token.type == TOKEN_TRUE, line);
        case TOKEN_LPAREN: {
            advance();
            ASTNode *expr = parse_expression();
            expect(TOKEN_RPAREN);
            return expr;
        }
        case TOKEN_NOT: {
            advance();
            ASTNode *operand = parse_factor();
            return ast_new_unary_expr(UNARY_NOT, operand, line);
        }
        case TOKEN_MINUS: {
            advance();
            ASTNode *operand = parse_factor();
            return ast_new_unary_expr(UNARY_MINUS, operand, line);
        }
        default:
            syntax_error(line, "Unexpected token in factor");
            advance();
            return ast_new_const_int(0, line);
    }
}