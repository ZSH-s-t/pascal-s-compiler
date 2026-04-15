/**
 * @file ast.c
 * @brief AST构造与操作实现
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 创建基础节点 */
ASTNode *ast_new_node(ASTNodeType type, int line) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    return node;
}

/* 标识符节点（用于idlist） */
ASTNode *ast_new_identifier(const char *name, int line) {
    ASTNode *node = ast_new_node(AST_IDENTIFIER, line);
    strncpy(node->data.id_node.name, name, 63);
    node->data.id_node.name[63] = '\0';
    node->data.id_node.next = NULL;
    return node;
}

/* 常量节点 */
ASTNode *ast_new_const_int(int val, int line) {
    ASTNode *node = ast_new_node(AST_CONST_VAL, line);
    node->data.const_val.token_type = TOKEN_INTEGER_CONST;
    node->data.const_val.int_val = val;
    return node;
}

ASTNode *ast_new_const_real(double val, int line) {
    ASTNode *node = ast_new_node(AST_CONST_VAL, line);
    node->data.const_val.token_type = TOKEN_REAL_CONST;
    node->data.const_val.real_val = val;
    return node;
}

ASTNode *ast_new_const_char(char val, int line) {
    ASTNode *node = ast_new_node(AST_CONST_VAL, line);
    node->data.const_val.token_type = TOKEN_CHAR_CONST;
    node->data.const_val.char_val = val;
    return node;
}

ASTNode *ast_new_const_bool(int val, int line) {
    ASTNode *node = ast_new_node(AST_CONST_VAL, line);
    node->data.const_val.token_type = val ? TOKEN_TRUE : TOKEN_FALSE;
    node->data.const_val.bool_val = val;
    return node;
}

/* 二元表达式 */
ASTNode *ast_new_binary_expr(BinaryOp op, ASTNode *left, ASTNode *right, int line) {
    ASTNode *node = ast_new_node(AST_BINARY_EXPR, line);
    node->data.binary.op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;
    return node;
}

/* 一元表达式 */
ASTNode *ast_new_unary_expr(UnaryOp op, ASTNode *operand, int line) {
    ASTNode *node = ast_new_node(AST_UNARY_EXPR, line);
    node->data.unary.op = op;
    node->data.unary.operand = operand;
    return node;
}

/* 变量引用 */
ASTNode *ast_new_var_ref(const char *name, ASTNode *index, int line) {
    ASTNode *node = ast_new_node(AST_VAR_REF, line);
    strncpy(node->data.var_ref.name, name, 63);
    node->data.var_ref.name[63] = '\0';
    node->data.var_ref.index_expr = index;
    return node;
}

/* 向语句列表追加语句 */
ASTNode *ast_append_stmt(ASTNode *list, ASTNode *stmt) {
    if (!list) return stmt;
    if (list->type != AST_STMT_LIST) {
        ASTNode *newlist = ast_new_node(AST_STMT_LIST, list->line);
        newlist->data.stmt_list.first = newlist->data.stmt_list.last = list;
        list = newlist;
    }
    if (!list->data.stmt_list.first) {
        list->data.stmt_list.first = list->data.stmt_list.last = stmt;
    } else {
        list->data.stmt_list.last->next = stmt;
        list->data.stmt_list.last = stmt;
    }
    return list;
}

/* 向声明链表追加声明 */
ASTNode *ast_append_decl(ASTNode *list, ASTNode *decl) {
    if (!list) return decl;
    ASTNode *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = decl;
    return list;
}

/* 向标识符链表追加标识符 */
ASTNode *ast_append_id(ASTNode *list, const char *name, int line) {
    ASTNode *id = ast_new_identifier(name, line);
    if (!list) return id;
    ASTNode *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = id;
    return list;
}

/* 向参数链表追加参数 */
ASTNode *ast_append_param(ASTNode *list, int is_var, ASTNode *id_list, DataType type, int line) {
    ASTNode *param = ast_new_node(AST_PARAM_LIST, line);
    param->data.param.is_var = is_var;
    param->data.param.id_list = id_list;
    param->data.param.type = type;
    param->data.param.next_param = NULL;
    if (!list) return param;
    ASTNode *cur = list;
    while (cur->data.param.next_param) cur = cur->data.param.next_param;
    cur->data.param.next_param = param;
    return list;
}

/* ========== AST打印（用于调试） ========== */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static const char *op_name(BinaryOp op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_DIV_INT: return "div";
        case OP_MOD: return "mod";
        case OP_AND: return "and";
        case OP_OR: return "or";
        case OP_EQ: return "=";
        case OP_NE: return "<>";
        case OP_LT: return "<";
        case OP_LE: return "<=";
        case OP_GT: return ">";
        case OP_GE: return ">=";
        default: return "?";
    }
}

static const char *type_name(DataType t) {
    switch (t) {
        case TYPE_INTEGER: return "integer";
        case TYPE_REAL: return "real";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_CHAR: return "char";
        case TYPE_ARRAY: return "array";
        default: return "unknown";
    }
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    print_indent(indent);
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program: %s (line %d)\n", node->data.program.prog_name, node->line);
            ast_print(node->data.program.const_decls, indent+1);
            ast_print(node->data.program.var_decls, indent+1);
            ast_print(node->data.program.subprog_decls, indent+1);
            ast_print(node->data.program.body, indent+1);
            break;
        case AST_VAR_DECL:
            printf("VarDecl (line %d): ", node->line);
            for (ASTNode *id = node->data.var_decl.id_list; id; id = id->next)
                printf("%s ", id->data.id_node.name);
            printf(": %s\n", type_name(node->data.var_decl.data_type));
            ast_print(node->data.var_decl.next_decl, indent);
            break;
        case AST_CONST_DECL:
            printf("ConstDecl (line %d): %s = ", node->line, node->data.const_decl.name);
            ast_print(node->data.const_decl.value, 0);
            ast_print(node->data.const_decl.next_decl, indent);
            break;
        case AST_SUBPROG_DECL:
            printf("%s (line %d): %s\n", 
                   node->data.subprog.is_function ? "Function" : "Procedure",
                   node->line, node->data.subprog.name);
            ast_print(node->data.subprog.body, indent+1);
            ast_print(node->data.subprog.next_decl, indent);
            break;
        case AST_COMPOUND_STMT:
            printf("CompoundStmt (line %d)\n", node->line);
            ast_print(node->data.compound.stmt_list, indent+1);
            break;
        case AST_ASSIGN_STMT:
            printf("Assign (line %d):\n", node->line);
            print_indent(indent+1); printf("LHS:\n");
            ast_print(node->data.assign.lhs, indent+2);
            print_indent(indent+1); printf("RHS:\n");
            ast_print(node->data.assign.rhs, indent+2);
            break;
        case AST_IF_STMT:
            printf("If (line %d):\n", node->line);
            print_indent(indent+1); printf("Cond:\n");
            ast_print(node->data.if_stmt.cond, indent+2);
            print_indent(indent+1); printf("Then:\n");
            ast_print(node->data.if_stmt.then_part, indent+2);
            if (node->data.if_stmt.else_part) {
                print_indent(indent+1); printf("Else:\n");
                ast_print(node->data.if_stmt.else_part, indent+2);
            }
            break;
        case AST_FOR_STMT:
            printf("For (line %d): %s := ", node->line, node->data.for_stmt.var_name);
            ast_print(node->data.for_stmt.start_expr, 0);
            printf(" to ");
            ast_print(node->data.for_stmt.end_expr, 0);
            printf("\n");
            ast_print(node->data.for_stmt.body, indent+1);
            break;
        case AST_BINARY_EXPR:
            printf("BinaryOp: %s (line %d)\n", op_name(node->data.binary.op), node->line);
            ast_print(node->data.binary.left, indent+1);
            ast_print(node->data.binary.right, indent+1);
            break;
        case AST_UNARY_EXPR:
            printf("UnaryOp: %s (line %d)\n",
                   node->data.unary.op == UNARY_NOT ? "not" : "-", node->line);
            ast_print(node->data.unary.operand, indent+1);
            break;
        case AST_VAR_REF:
            printf("VarRef: %s", node->data.var_ref.name);
            if (node->data.var_ref.index_expr) {
                printf("[");
                ast_print(node->data.var_ref.index_expr, 0);
                printf("]");
            }
            printf(" (line %d)\n", node->line);
            break;
        case AST_CONST_VAL:
            switch (node->data.const_val.token_type) {
                case TOKEN_INTEGER_CONST: printf("%d", node->data.const_val.int_val); break;
                case TOKEN_REAL_CONST: printf("%g", node->data.const_val.real_val); break;
                case TOKEN_CHAR_CONST: printf("'%c'", node->data.const_val.char_val); break;
                case TOKEN_TRUE: printf("true"); break;
                case TOKEN_FALSE: printf("false"); break;
                default: printf("?");
            }
            printf(" (line %d)\n", node->line);
            break;
        case AST_STMT_LIST:
            for (ASTNode *s = node->data.stmt_list.first; s; s = s->next)
                ast_print(s, indent);
            break;
        default:
            printf("<Node type %d> (line %d)\n", node->type, node->line);
    }
}

/* 释放AST内存 */
void ast_free(ASTNode *node) {
    if (!node) return;
    /* 递归释放子节点 */
    switch (node->type) {
        case AST_PROGRAM:
            ast_free(node->data.program.const_decls);
            ast_free(node->data.program.var_decls);
            ast_free(node->data.program.subprog_decls);
            ast_free(node->data.program.body);
            break;
        case AST_VAR_DECL:
            ast_free(node->data.var_decl.id_list);
            ast_free(node->data.var_decl.array_bounds);
            ast_free(node->data.var_decl.next_decl);
            break;
        case AST_CONST_DECL:
            ast_free(node->data.const_decl.value);
            ast_free(node->data.const_decl.next_decl);
            break;
        case AST_SUBPROG_DECL:
            ast_free(node->data.subprog.params);
            ast_free(node->data.subprog.body);
            ast_free(node->data.subprog.next_decl);
            break;
        case AST_COMPOUND_STMT:
            ast_free(node->data.compound.stmt_list);
            break;
        case AST_ASSIGN_STMT:
            ast_free(node->data.assign.lhs);
            ast_free(node->data.assign.rhs);
            break;
        case AST_IF_STMT:
            ast_free(node->data.if_stmt.cond);
            ast_free(node->data.if_stmt.then_part);
            ast_free(node->data.if_stmt.else_part);
            break;
        case AST_FOR_STMT:
            ast_free(node->data.for_stmt.start_expr);
            ast_free(node->data.for_stmt.end_expr);
            ast_free(node->data.for_stmt.body);
            break;
        case AST_BINARY_EXPR:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case AST_UNARY_EXPR:
            ast_free(node->data.unary.operand);
            break;
        case AST_VAR_REF:
            ast_free(node->data.var_ref.index_expr);
            break;
        case AST_STMT_LIST:
            for (ASTNode *s = node->data.stmt_list.first; s; ) {
                ASTNode *next = s->next;
                ast_free(s);
                s = next;
            }
            break;
        case AST_IDENTIFIER:
            ast_free(node->data.id_node.next);
            break;
        case AST_PARAM_LIST:
            ast_free(node->data.param.id_list);
            ast_free(node->data.param.next_param);
            break;
        default:
            break;
    }
    free(node);
}