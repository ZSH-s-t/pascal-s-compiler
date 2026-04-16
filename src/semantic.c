/**
 * @file semantic.c
 * @brief 语义分析器实现
 * @author 陈妍语
 */


#include "semantic.h"
#include "symbol.h"
#include "ast.h"
#include "lexer.h"
#include "error.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h> 

static int error_count = 0;

/* 报告语义错误 */
static void report_error(int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[Semantic Error] Line %d: ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    error_count++;
}

// semantic.c - 修改类型兼容性检查
static int is_type_compatible(DataType a, DataType b) {
    if (a == b) return 1;
    // integer 可以隐式转换为 real
    if (a == TYPE_INTEGER && b == TYPE_REAL) return 1;
    // 注意：real 不能隐式转换为 integer
    return 0;
}

// 添加赋值时的类型检查（更严格）
static int is_assign_compatible(DataType lhs, DataType rhs) {
    if (lhs == rhs) return 1;
    // integer 可以赋值给 real
    if (rhs == TYPE_INTEGER && lhs == TYPE_REAL) return 1;
    return 0;
}

/* 获取类型名称 */
static const char* type_name(DataType t) {
    switch (t) {
        case TYPE_INTEGER: return "integer";
        case TYPE_REAL: return "real";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_CHAR: return "char";
        case TYPE_ARRAY: return "array";
        default: return "unknown";
    }
}

/* 检查表达式类型（前向声明） */
static ExprType check_expr(ASTNode* expr);

/* 检查二元表达式 */
static ExprType check_binary_expr(ASTNode* node) {
    ExprType left = check_expr(node->data.binary.left);
    ExprType right = check_expr(node->data.binary.right);
    ExprType result = {TYPE_UNKNOWN, 0, 0};
    
    BinaryOp op = node->data.binary.op;
    
    /* 关系运算符返回boolean */
    if (op >= OP_EQ && op <= OP_GE) {
        if (is_type_compatible(left.type, right.type) ||
            is_type_compatible(right.type, left.type)) {
            result.type = TYPE_BOOLEAN;
        } else {
            report_error(node->line, "Cannot compare %s with %s",
                        type_name(left.type), type_name(right.type));
        }
        return result;
    }
    
    /* 逻辑运算符（and, or）要求boolean */
    if (op == OP_AND || op == OP_OR) {
        if (left.type != TYPE_BOOLEAN) {
            report_error(node->line, "Left operand of '%s' must be boolean, got %s",
                        op == OP_AND ? "and" : "or", type_name(left.type));
        }
        if (right.type != TYPE_BOOLEAN) {
            report_error(node->line, "Right operand of '%s' must be boolean, got %s",
                        op == OP_AND ? "and" : "or", type_name(right.type));
        }
        result.type = TYPE_BOOLEAN;
        return result;
    }
    
    /* 算术运算符 */
    if (left.type == TYPE_INTEGER && right.type == TYPE_INTEGER) {
        result.type = TYPE_INTEGER;
        if (op == OP_DIV) {
            /* '/' 在Pascal中除法结果是real */
            result.type = TYPE_REAL;
        }
    } else if ((left.type == TYPE_INTEGER || left.type == TYPE_REAL) &&
               (right.type == TYPE_INTEGER || right.type == TYPE_REAL)) {
        result.type = TYPE_REAL;
    } else {
        report_error(node->line, "Arithmetic operands must be numeric, got %s and %s",
                    type_name(left.type), type_name(right.type));
    }
    
    return result;
}

/* 检查一元表达式 */
static ExprType check_unary_expr(ASTNode* node) {
    ExprType operand = check_expr(node->data.unary.operand);
    ExprType result = {TYPE_UNKNOWN, 0, 0};
    
    if (node->data.unary.op == UNARY_NOT) {
        if (operand.type != TYPE_BOOLEAN) {
            report_error(node->line, "'not' operator requires boolean operand, got %s",
                        type_name(operand.type));
        }
        result.type = TYPE_BOOLEAN;
    } else if (node->data.unary.op == UNARY_MINUS) {
        if (operand.type != TYPE_INTEGER && operand.type != TYPE_REAL) {
            report_error(node->line, "Unary minus requires numeric operand, got %s",
                        type_name(operand.type));
        }
        result.type = operand.type;
    }
    
    return result;
}


/* 检查函数调用表达式 */
static ExprType check_call_expr(ASTNode* node) {
    SymEntry* sym = lookup_symbol(node->data.call_expr.name);
    
    if (!sym) {
        report_error(node->line, "Undeclared function '%s'", node->data.call_expr.name);
        return (ExprType){TYPE_UNKNOWN, 0, 0};
    }
    
    if (sym->kind != SYM_FUNC) {
        report_error(node->line, "'%s' is not a function", node->data.call_expr.name);
        return (ExprType){TYPE_UNKNOWN, 0, 0};
    }
    
    /* 检查参数数量和类型 (简化版本) */
    ASTNode* args_list = node->data.call_expr.args;
    ASTNode* arg = NULL;
    
    /* parse_expression_list返回AST_STMT_LIST节点 */
    if (args_list && args_list->type == AST_STMT_LIST) {
        arg = args_list->data.stmt_list.first;
    } else {
        arg = args_list;
    }
    
    int arg_count = 0;
    while (arg) {
        arg_count++;
        check_expr(arg);
        arg = arg->next;
    }
    
    /* 返回函数的返回类型 */
    return (ExprType){sym->type, 0, 0};
}

/* 检查变量引用 */
static ExprType check_var_ref(ASTNode* node) {
    SymEntry* sym = lookup_symbol(node->data.var_ref.name);
    
    if (!sym) {
        report_error(node->line, "Undeclared identifier '%s'", node->data.var_ref.name);
        return (ExprType){TYPE_UNKNOWN, 0, 0};
    }
    
    /* 检查是否为函数（函数名可以出现在表达式中） */
    if (sym->kind == SYM_FUNC) {
        /* 函数调用，返回函数类型 */
        return (ExprType){sym->type, 0, 0};
    }
    
    if (sym->kind != SYM_VAR && sym->kind != SYM_CONST) {
        report_error(node->line, "'%s' is not a variable or constant", node->data.var_ref.name);
        return (ExprType){TYPE_UNKNOWN, 0, 0};
    }
    
    /* 处理数组下标 */
    if (sym->type == TYPE_ARRAY) {
        if (node->data.var_ref.index_expr == NULL) {
            report_error(node->line, "Array '%s' requires subscript", node->data.var_ref.name);
            return (ExprType){TYPE_UNKNOWN, 0, 0};
        }
        ExprType index_type = check_expr(node->data.var_ref.index_expr);
        if (index_type.type != TYPE_INTEGER) {
            report_error(node->line, "Array subscript must be integer, got %s",
                        type_name(index_type.type));
        }
        return (ExprType){sym->u.array_info.elem_type, 0, 0};
    } else {
        if (node->data.var_ref.index_expr != NULL) {
            report_error(node->line, "Non-array '%s' used with subscript", node->data.var_ref.name);
        }
        if (sym->kind == SYM_CONST) {
            return (ExprType){sym->type, 1, sym->u.const_value};
        }
        return (ExprType){sym->type, 0, 0};
    }
}

/* 检查常量值 */
static ExprType check_const_val(ASTNode* node) {
    ExprType result = {TYPE_UNKNOWN, 1, 0};
    
    switch (node->data.const_val.token_type) {
        case TOKEN_INTEGER_CONST:
            result.type = TYPE_INTEGER;
            result.const_value = node->data.const_val.int_val;
            break;
        case TOKEN_REAL_CONST:
            result.type = TYPE_REAL;
            break;
        case TOKEN_CHAR_CONST:
            result.type = TYPE_CHAR;
            result.const_value = node->data.const_val.char_val;
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            result.type = TYPE_BOOLEAN;
            result.const_value = (node->data.const_val.token_type == TOKEN_TRUE);
            break;
        default:
            break;
    }
    return result;
}

/* 检查表达式（主函数） */
static ExprType check_expr(ASTNode* expr) {
    if (!expr) return (ExprType){TYPE_UNKNOWN, 0, 0};
    
    switch (expr->type) {
        case AST_BINARY_EXPR:
            return check_binary_expr(expr);
        case AST_UNARY_EXPR:
            return check_unary_expr(expr);
        case AST_VAR_REF:
            return check_var_ref(expr);
        case AST_CALL_EXPR:
            return check_call_expr(expr);
        case AST_CONST_VAL:
            return check_const_val(expr);
        default:
            report_error(expr->line, "Invalid expression node type %d", expr->type);
            return (ExprType){TYPE_UNKNOWN, 0, 0};
    }
}

/* 检查赋值语句 */
static void check_assign_stmt(ASTNode* node) {
    /* 检查左值 */
    if (node->data.assign.lhs->type != AST_VAR_REF) {
        report_error(node->line, "Left side of assignment must be a variable");
        return;
    }
    
    SymEntry* lhs_sym = lookup_symbol(node->data.assign.lhs->data.var_ref.name);
    if (!lhs_sym) {
        report_error(node->line, "Undeclared variable '%s'", 
                    node->data.assign.lhs->data.var_ref.name);
        return;
    }
    
    if (lhs_sym->kind == SYM_CONST) {
        report_error(node->line, "Cannot assign to constant '%s'", lhs_sym->name);
        return;
    }
    
    /* 获取左值类型 */
    DataType lhs_type = lhs_sym->type;
    if (lhs_type == TYPE_ARRAY) {
        lhs_type = lhs_sym->u.array_info.elem_type;
    }
    
    /* 检查右值 */
    ExprType rhs = check_expr(node->data.assign.rhs);
    
    /* 类型兼容性检查 */
    if (!is_type_compatible(rhs.type, lhs_type)) {
        report_error(node->line, "Type mismatch: cannot assign %s to %s",
                    type_name(rhs.type), type_name(lhs_type));
    }
    
    /* 如果是函数赋值（函数名出现在赋值左边） */
    if (lhs_sym->kind == SYM_FUNC) {
        /* 在函数内部给函数名赋值 */
        lhs_sym->u.func_info.has_return = 1;
    }
}

/* 检查if语句 */
static void check_if_stmt(ASTNode* node) {
    ExprType cond = check_expr(node->data.if_stmt.cond);
    if (cond.type != TYPE_BOOLEAN) {
        report_error(node->line, "If condition must be boolean, got %s",
                    type_name(cond.type));
    }
    /* 递归检查then和else部分（在语句检查中处理）*/
}

/* 检查for语句 */
static void check_for_stmt(ASTNode* node) {
    SymEntry* var = lookup_symbol(node->data.for_stmt.var_name);
    if (!var) {
        report_error(node->line, "Undeclared loop variable '%s'",
                    node->data.for_stmt.var_name);
    } else if (var->type != TYPE_INTEGER) {
        report_error(node->line, "For loop variable must be integer, got %s",
                    type_name(var->type));
    }
    
    ExprType start = check_expr(node->data.for_stmt.start_expr);
    ExprType end = check_expr(node->data.for_stmt.end_expr);
    
    if (start.type != TYPE_INTEGER) {
        report_error(node->line, "For loop start value must be integer, got %s",
                    type_name(start.type));
    }
    if (end.type != TYPE_INTEGER) {
        report_error(node->line, "For loop end value must be integer, got %s",
                    type_name(end.type));
    }
}

/* 检查语句（前向声明） */
static void check_statement(ASTNode* stmt);

/* 检查语句列表 */
static void check_statement_list(ASTNode* list) {
    if (!list || list->type != AST_STMT_LIST) return;
    
    for (ASTNode* stmt = list->data.stmt_list.first; stmt; stmt = stmt->next) {
        check_statement(stmt);
    }
}

/* 检查复合语句 */
static void check_compound_stmt(ASTNode* node) {
    check_statement_list(node->data.compound.stmt_list);
}

/* 检查过程调用 */
static void check_call_stmt(ASTNode* node) {
    /* 简化版本：假设调用的是已声明的过程 */
    /* 实际需要从AST中获取被调用的函数名 */
}

/* 检查read语句 */
static void check_read_stmt(ASTNode* node) {
    /* read语句中的变量检查在代码生成阶段完成 */
}

/* 检查write语句 */
static void check_write_stmt(ASTNode* node) {
    /* write语句中的表达式检查在表达式检查中完成 */
}

/* 检查单个语句 */
static void check_statement(ASTNode* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_ASSIGN_STMT:
            check_assign_stmt(stmt);
            break;
        case AST_IF_STMT:
            check_if_stmt(stmt);
            check_statement(stmt->data.if_stmt.then_part);
            if (stmt->data.if_stmt.else_part) {
                check_statement(stmt->data.if_stmt.else_part);
            }
            break;
        case AST_FOR_STMT:
            check_for_stmt(stmt);
            check_statement(stmt->data.for_stmt.body);
            break;
        case AST_COMPOUND_STMT:
            check_compound_stmt(stmt);
            break;
        case AST_CALL_STMT:
            check_call_stmt(stmt);
            break;
        case AST_READ_STMT:
            check_read_stmt(stmt);
            break;
        case AST_WRITE_STMT:
            check_write_stmt(stmt);
            break;
        default:
            break;
    }
}

/* 处理常量声明 */
/* 处理常量声明 */
static void process_const_decl(ASTNode* node) {
    if (!node) return;
    
    if (node->type == AST_CONST_DECL) {
        //printf("[DEBUG] ConstDecl: %s\n", node->data.const_decl.name);
        int value = 0;
        if (node->data.const_decl.value && 
            node->data.const_decl.value->type == AST_CONST_VAL) {
            value = node->data.const_decl.value->data.const_val.int_val;
        }
        add_const(node->data.const_decl.name, value, TYPE_INTEGER, node->line);
        
        /* 处理下一个常量声明 */
        if (node->data.const_decl.next_decl) {
            process_const_decl(node->data.const_decl.next_decl);
        }
    }
}

/* 处理变量声明 */
/* 处理变量声明 */
static void process_var_decl(ASTNode* node) {
    if (!node) return;
    
    // 打印调试信息（确认节点类型）
    //printf("[DEBUG] process_var_decl: node type = %d\n", node->type);
    
    if (node->type == AST_VAR_DECL) {
        //printf("[DEBUG] var_decl: data_type = %d\n", node->data.var_decl.data_type);
        
        // 打印 id_list 的内容
        ASTNode* id = node->data.var_decl.id_list;
        //printf("[DEBUG] id_list: ");
        while (id) {
            printf("%s ", id->data.id_node.name);
            id = id->next;
        }
        printf("\n");
        
        // 遍历标识符列表，添加每个变量
        id = node->data.var_decl.id_list;
        while (id) {
            if (id->type == AST_IDENTIFIER) {
                //printf("[DEBUG] Adding variable: %s\n", id->data.id_node.name);
                if (node->data.var_decl.data_type == TYPE_ARRAY) {
                    ArrayInfo info = {0};
                    info.low = 1;
                    info.high = 10;
                    info.elem_type = TYPE_INTEGER;
                    add_array(id->data.id_node.name, &info, node->line);
                } else {
                    add_var(id->data.id_node.name, node->data.var_decl.data_type, node->line);
                }
            }
            id = id->next;
        }
        
        // 处理下一个声明（如果有多个 var 声明块）
        if (node->data.var_decl.next_decl) {
            process_var_decl(node->data.var_decl.next_decl);
        }
    }
}

/* 处理子程序声明 */
/* 处理子程序声明 */
/* 处理子程序声明 */
static void process_subprog_decl(ASTNode* node) {
    if (!node) return;
    
    if (node->type == AST_SUBPROG_DECL) {
        //printf("[DEBUG] process_subprog_decl: %s\n", node->data.subprog.name);
        
        /* ========== 关键修复：先添加函数/过程名到当前作用域（外层） ========== */
        if (node->data.subprog.is_function) {
            add_func(node->data.subprog.name, NULL, 0,
                    node->data.subprog.return_type, node->line);
            //printf("[DEBUG] After add_func, looking up: %s\n", node->data.subprog.name);
             SymEntry* test = lookup_symbol(node->data.subprog.name);
            //printf("[DEBUG] lookup result: %s\n", test ? test->name : "NULL");
        } else {
            add_proc(node->data.subprog.name, NULL, 0, node->line);
        }
        
        /* 进入新作用域（函数内部） */
        enter_scope();
        
        /* 添加参数到符号表（在函数作用域内） */
        ASTNode* param = node->data.subprog.params;
        while (param && param->type == AST_PARAM_LIST) {
            //printf("[DEBUG] param: is_var=%d, type=%d\n", param->data.param.is_var, param->data.param.type);
            
            ASTNode* id_list = param->data.param.id_list;
            while (id_list && id_list->type == AST_IDENTIFIER) {
                //printf("[DEBUG] Adding parameter: %s\n", id_list->data.id_node.name);
                add_var(id_list->data.id_node.name, param->data.param.type, node->line);
                id_list = id_list->next;
            }
            param = param->data.param.next_param;
        }
        
        /* 检查子程序体中的语句 */
        check_statement(node->data.subprog.body);
        
        /* 如果是函数，检查是否有返回值 */
        SymEntry* func = lookup_symbol(node->data.subprog.name);
        if (func && func->kind == SYM_FUNC && !func->u.func_info.has_return) {
            report_error(node->line, "Function '%s' may not return a value", func->name);
        }
        
        /* 退出作用域 */
        exit_scope();
        
        /* 处理下一个子程序 */
        if (node->data.subprog.next_decl) {
            process_subprog_decl(node->data.subprog.next_decl);
        }
    }
}

/* 主语义分析函数 */
SemanticResult semantic_analyze(ASTNode* ast) {
    error_count = 0;
    
    if (!ast) {
        return (SemanticResult){1, 1};
    }
    
    printf("\n=== Semantic Analysis Started ===\n");
    
    /* 打印AST用于调试 */
    //printf("\n[DEBUG] AST Structure:\n");
    ast_print(ast, 0);
    printf("\n");
    
    /* 初始化符号表 */
    init_symtable();
    
    /* 处理常量声明 */
    if (ast->type == AST_PROGRAM) {
        //printf("[DEBUG] Processing const declarations\n");
        process_const_decl(ast->data.program.const_decls);
        
        //printf("[DEBUG] Processing var declarations\n");
        process_var_decl(ast->data.program.var_decls);
        
        //printf("[DEBUG] Processing subprogram declarations\n");
        process_subprog_decl(ast->data.program.subprog_decls);
        
        //printf("[DEBUG] Checking main program body\n");
        /* 检查主程序体 */
        check_statement(ast->data.program.body);
    }
    
    /* 打印符号表 */
    print_symtable(stdout);
    
    printf("=== Semantic Analysis Complete ===\n");
    if (error_count > 0) {
        printf("Found %d semantic error(s)\n", error_count);
    } else {
        printf("No semantic errors detected.\n");
    }
    
    /* 释放符号表 */
    free_symtable();
    
    return (SemanticResult){error_count > 0, error_count};
}