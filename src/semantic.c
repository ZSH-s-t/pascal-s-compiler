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

// 语义分析错误计数器
static int error_count = 0;
/* 语义分析输入 */
static ASTNode* ast = NULL;

// 获取调试等级（根据 verbose 模式）
static int get_debug_level(void) {
    return g_verbose ? 1 : 0;
}

// 前向声明
static void report_error(int line, const char* fmt, ...);
static void process_const_decl(ASTNode* node);
static void process_var_decl(ASTNode* node);
static void process_subprog_decl(ASTNode* node);
static void check_statement(ASTNode* node);
static ExprType check_expr(ASTNode* expr);
static void check_assign_stmt(ASTNode* node);
static void check_if_stmt(ASTNode* node);
static void check_for_stmt(ASTNode* node);
static void check_compound_stmt(ASTNode* node);
static void check_read_stmt(ASTNode* node);
static void check_write_stmt(ASTNode* node);
static ExprType check_binary_expr(ASTNode* node);
static ExprType check_unary_expr(ASTNode* node);
static ExprType check_var_ref(ASTNode* node);
static ExprType check_const_val(ASTNode* node);
static ExprType check_call_expr(ASTNode* node);
static void debug_content(const char* fmt, ...);

//====================================== 主入口函数 ======================================
/* 0. 主语义分析函数
 * @param ast 语法树根节点
 * @return SemanticResult 语义分析结果
 * @note 该函数会打印语义分析结果以及符号表结果，包括错误信息
 */
SemanticResult semantic_analyze(ASTNode* input_ast) {
    error_count = 0;
    ast = input_ast;
    
    // [错误检测]检查语法树根节点是否为空
    debug_content("检测语法树根节点是否为空");
    if (!ast) {
        report_error(0, "AST is empty,no semantic analysis performed.");
        return (SemanticResult){1, 1};
    }
    
    debug_content("=== 语义分析开始 ===\n");
    
    /* 初始化符号表 */
    init_symtable();
    
    /* 处理常量声明 */
    if (ast->type == AST_PROGRAM) {
        debug_content("Processing constant declarations");
        //1. 处理常量声明
        process_const_decl(ast->data.program.const_decls);
        
        debug_content("Processing var declarations");
        //2. 处理变量声明
        process_var_decl(ast->data.program.var_decls);
        
        debug_content("Processing subprogram declarations");
        //3. 处理子程序声明
        process_subprog_decl(ast->data.program.subprog_decls);
        
        debug_content("Checking main program body");
        //4. 检查主程序体
        check_statement(ast->data.program.body);
    }
    
    /* 打印符号表 */
    if(get_debug_level() >= 1)
        print_symtable(stdout);
    
    debug_content("=== Semantic Analysis Complete ===\n");
    if (error_count > 0) {
        debug_content("Found %d semantic error(s)\n", error_count);
    } else {
        debug_content("No semantic errors detected.\n");
    }
    
    /*符号表不在这里释放，等到代码生成结束后再释放 */
    
    return (SemanticResult){error_count > 0, error_count};
}





//====================================== 调试函数 ======================================
/* 调试函数：用于输出所有调试信息 */
static void debug(DebugContent debug_content, const char* fmt){
    if(get_debug_level() == 0){
        return;
    }
    printf("[semantic debug]: ");
    switch (debug_content)
    {
        case PRINT_AST:
            ast_print(ast, 0);
            printf("\n");
            break;
        case DEBUG_CONTENT:
            printf("%s\n", fmt);
            break;
        default:
            break;
    }
}

/* 调试函数：用于输出手动调试的信息（支持可变参数，类似 printf） */
static void debug_content(const char* fmt, ...) {
    if(get_debug_level() == 0) return;
    
    va_list args;
    va_start(args, fmt);
    
    // 使用动态缓冲区
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    debug(DEBUG_CONTENT, buffer);
    
    va_end(args);
}

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





//====================================== 工具函数 ======================================
// 修改类型兼容性检查
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




//====================================== 检查语句的函数 ======================================
/* 1. 检查单个语句 */
static void check_statement(ASTNode* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_ASSIGN_STMT:// 赋值语句
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
        case AST_WHILE_STMT: {
            ExprType cond_type = check_expr(stmt->data.while_stmt.cond);
            if (cond_type.type != TYPE_BOOLEAN && cond_type.type != TYPE_INTEGER) {
                report_error(stmt->line, "While condition must be boolean or integer, got %s",
                            type_name(cond_type.type));
            }
            check_statement(stmt->data.while_stmt.body);
            break;
        }
        case AST_COMPOUND_STMT:
            check_compound_stmt(stmt);
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

/* 2. 检查赋值语句 */
static void check_assign_stmt(ASTNode* node) {
    /* 检查左值 */
    if (node->data.assign.lhs->type != AST_VAR_REF) {// 左值必须是变量引用
        report_error(node->line, "Left side of assignment must be a variable");
        return;
    }
    
    const char* lhs_name = node->data.assign.lhs->data.var_ref.name;
    SymEntry* lhs_sym = lookup_symbol(lhs_name);
    
    if (!lhs_sym) {// 左值必须是已声明的变量
        report_error(node->line, "Undeclared variable '%s'", lhs_name);
        return;
    }
    
    /* 函数名赋值（函数返回值） */
    if (lhs_sym->kind == SYM_FUNC) {
        /* 在函数内部给函数名赋值，标记为有返回值 */
        lhs_sym->u.func_info.has_return = 1;
        debug_content(lhs_name," has return value assignment\n");
        return;
    }
    
    if (lhs_sym->kind == SYM_CONST) {// 常量不能赋值
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
    if (!is_assign_compatible(lhs_type, rhs.type)) {
        report_error(node->line, "Type mismatch: cannot assign %s to %s",
                    type_name(rhs.type), type_name(lhs_type));
    }
}

/* 3. 检查表达式（主函数） */
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

/* 检查二元表达式 */
static ExprType check_binary_expr(ASTNode* node) {
    ExprType left = check_expr(node->data.binary.left);
    ExprType right = check_expr(node->data.binary.right);
    ExprType result = {TYPE_UNKNOWN, 0, 0};
    
    BinaryOp op = node->data.binary.op;
    
    /* 关系运算符返回 boolean */
    if (op >= OP_EQ && op <= OP_GE) {
        if (is_assign_compatible(left.type, right.type) ||
            is_assign_compatible(right.type, left.type)) {
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
        /* not 运算符可以应用于 boolean 或 integer */
        if (operand.type == TYPE_BOOLEAN) {
            result.type = TYPE_BOOLEAN;
        } else if (operand.type == TYPE_INTEGER) {
            /* 按位取反，结果仍为 integer */
            result.type = TYPE_INTEGER;
        } else {
            report_error(node->line, "'not' operator requires boolean or integer operand, got %s",
                        type_name(operand.type));
        }
    } else if (node->data.unary.op == UNARY_MINUS) {
        if (operand.type != TYPE_INTEGER && operand.type != TYPE_REAL) {
            report_error(node->line, "Unary minus requires numeric operand, got %s",
                        type_name(operand.type));
        }
        result.type = operand.type;
    }
    
    return result;
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
/* 处理多维数组下标（可能是 AST_STMT_LIST 包含多个下标） */
        ASTNode* index = node->data.var_ref.index_expr;
        if (index->type == AST_STMT_LIST) {
            /* 多维数组：检查每个下标 */
            ASTNode* idx = index->data.stmt_list.first;
            while (idx) {
                ExprType index_type = check_expr(idx);
                if (index_type.type != TYPE_INTEGER) {
                    report_error(node->line, "Array subscript must be integer, got %s",
                                type_name(index_type.type));
                }
                idx = idx->next;
            }
        } else {
            /* 单维数组 */
            ExprType index_type = check_expr(index);
            if (index_type.type != TYPE_INTEGER) {
                report_error(node->line, "Array subscript must be integer, got %s",
                            type_name(index_type.type));
            }
        }
        return (ExprType){sym->u.array_info.elem_type, 0, 0};
    } else {
        if (node->data.var_ref.index_expr != NULL) {
            report_error(node->line, "Non-array '%s' used with subscript", node->data.var_ref.name);
        }
        if (sym->kind == SYM_CONST) {
            int const_val = 0;
            switch (sym->type) {
                case TYPE_INTEGER:
                    const_val = sym->u.const_value.int_val;
                    break;
                case TYPE_REAL:
                    const_val = (int)sym->u.const_value.real_val;
                    break;
                case TYPE_CHAR:
                    const_val = sym->u.const_value.char_val;
                    break;
                case TYPE_BOOLEAN:
                    const_val = sym->u.const_value.bool_val;
                    break;
                default:
                    break;
            }
            return (ExprType){sym->type, 1, const_val};
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
            result.const_value = (int)node->data.const_val.real_val;
            break;
        case TOKEN_CHAR_CONST:
            result.type = TYPE_CHAR;
            result.const_value = node->data.const_val.char_val;
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            result.type = TYPE_BOOLEAN;
            result.const_value = (node->data.const_val.token_type == TOKEN_TRUE) ? 1 : 0;
            break;
        default:
            break;
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
    
    /* 检查参数数量 */
    ASTNode* args_list = node->data.call_expr.args;
    int arg_count = 0;
    ASTNode* arg = NULL;
    
    debug_content("function=%s, args_list=%p, type=%d", 
           node->data.call_expr.name, args_list, args_list ? args_list->type : -1);
    
    if (args_list) {
        if (args_list->type == AST_STMT_LIST) {
            debug_content("args_list is AST_STMT_LIST");
            arg = args_list->data.stmt_list.first;
        } else {
            debug_content("args_list is direct expression, type=%d", args_list->type);
            arg = args_list;
        }
        
        while (arg) {
            arg_count++;
            debug_content("arg %d: type=%d", arg_count, arg->type);
            check_expr(arg);
            arg = arg->next;
        }
    }
    
    debug_content("arg_count=%d, param_count=%d", arg_count, sym->u.func_info.param_count);
    
    if (arg_count != sym->u.func_info.param_count) {
        report_error(node->line, "Function '%s' expects %d argument(s), but got %d",
                    node->data.call_expr.name, sym->u.func_info.param_count, arg_count);
    }
    
    /* 返回函数的返回类型 */
    return (ExprType){sym->type, 0, 0};
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

/* 检查read语句 */
static void check_read_stmt(ASTNode* node) {
    /* read语句中的变量检查在代码生成阶段完成 */
}

/* 检查write语句 */
static void check_write_stmt(ASTNode* node) {
    /* write语句中的表达式检查在表达式检查中完成 */
}




//====================================== 构建符号表函数 ======================================
/* 1. 处理常量声明 */
static void process_const_decl(ASTNode* node) {
    if (!node){
        debug_content("No constant declaration found");
        return;
    }
    
    if (node->type == AST_CONST_DECL) {
        debug_content("Processing constant declaration: %s", node->data.const_decl.name);

        if (node->data.const_decl.value && 
            node->data.const_decl.value->type == AST_CONST_VAL) {
            
            ASTNode* value_node = node->data.const_decl.value;
            TokenType token_type = value_node->data.const_val.token_type;
            
            switch (token_type) {
                case TOKEN_INTEGER_CONST: {// 整数常量
                    int value = value_node->data.const_val.int_val;
                    add_const_int(node->data.const_decl.name, value, node->line);
                    debug_content("  Added integer constant: %s = %d", 
                                  node->data.const_decl.name, value);
                    break;
                }
                case TOKEN_REAL_CONST: {// 实数常量
                    double value = value_node->data.const_val.real_val;
                    add_const_real(node->data.const_decl.name, value, node->line);
                    debug_content("  Added real constant: %s = %f", 
                                  node->data.const_decl.name, value);
                    break;
                }
                case TOKEN_CHAR_CONST: {// 字符常量
                    char value = value_node->data.const_val.char_val;
                    add_const_char(node->data.const_decl.name, value, node->line);
                    debug_content("  Added char constant: %s = '%c'", 
                                  node->data.const_decl.name, value);
                    break;
                }
                case TOKEN_TRUE:// 布尔常量 true TODO
                case TOKEN_FALSE: {// 布尔常量 false TODO
                    int value = (token_type == TOKEN_TRUE) ? 1 : 0;
                    add_const_bool(node->data.const_decl.name, value, node->line);
                    debug_content("  Added boolean constant: %s = %s (token_type=%d)", 
                                  node->data.const_decl.name, value ? "true" : "false", token_type);
                    break;
                }
                default:// 未知常量类型
                    debug_content("  Warning: Unknown constant type for '%s', using integer", 
                                  node->data.const_decl.name);
                    add_const_int(node->data.const_decl.name, 0, node->line);
                    break;
            }
        }else {
            debug_content("  Warning: Invalid constant value for '%s'", node->data.const_decl.name);
            add_const_int(node->data.const_decl.name, 0, node->line);
        }
        
        if (node->data.const_decl.next_decl) {
            process_const_decl(node->data.const_decl.next_decl);
        }
    }
}

/* 2. 处理变量声明 */
static void process_var_decl(ASTNode* node) {
    if (!node){
        debug_content("No var declaration found");
        return;
    }
    
    if (node->type == AST_VAR_DECL) {
        // 打印 var 名称
        ASTNode* id = node->data.var_decl.id_list;
        debug_content("id_list: ");
        while (id) {
            debug_content(id->data.id_node.name," ");
            id = id->next;
        }
        
        // 遍历标识符列表，添加每个变量
        id = node->data.var_decl.id_list;
        while (id) {
            if (id->type == AST_IDENTIFIER) {
                debug_content("Adding variable:", id->data.id_node.name);
                if (node->data.var_decl.array_bounds != NULL) {
                    // 处理数组声明（有 array_bounds 说明是数组）
                    ArrayInfo info = {0};
                    
                    // 从 array_bounds 获取数组边界
                    if (node->data.var_decl.array_bounds) {
                        ASTNode* bounds = node->data.var_decl.array_bounds;
                        
                        // 获取下界和上界
                        ASTNode* low_expr = bounds->data.stmt_list.first;
                        ASTNode* high_expr = bounds->data.stmt_list.last;
                        
                        if (low_expr && low_expr->type == AST_CONST_VAL) {
                            info.low = low_expr->data.const_val.int_val;
                        } else {
                            info.low = LOW_ARRAY_BOUND; // 默认下界
                        }
                        
                        if (high_expr && high_expr->type == AST_CONST_VAL) {
                            info.high = high_expr->data.const_val.int_val;
                        } else {
                            info.high = HIGH_ARRAY_BOUND; // 默认上界
                        }
                    } else {
                        info.low = LOW_ARRAY_BOUND;
                        info.high = HIGH_ARRAY_BOUND;
                    }
                    
                    // 数组元素类型：由 parser 模块决定，存储在 elem_type 中
                    // 例如：array [1..10] of integer，parser 会将 elem_type 设为 TYPE_INTEGER
                    info.elem_type = node->data.var_decl.elem_type;
                    
                    add_array(id->data.id_node.name, &info, node->line);
                } else {
                    // 普通变量
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

/* 3. 处理子程序声明 */
static void process_subprog_decl(ASTNode* node) {
    if (!node) {
        debug_content("process_subprog_decl: node is NULL");
        return;
    }
    
    if (node->type == AST_SUBPROG_DECL) {
        debug_content("Processing subprogram:",node->data.subprog.name," (is_function=%d)", node->data.subprog.is_function);
        
        /* 添加函数/过程名到当前作用域（外层） */
        if (node->data.subprog.is_function) {
            add_func(node->data.subprog.name, NULL, 0,node->data.subprog.return_type, node->line);
            debug_content("  Added function: ", node->data.subprog.name);
        } else {
            add_proc(node->data.subprog.name, NULL, 0, node->line);
            debug_content("  Added procedure: ", node->data.subprog.name);
        }
        
        /* 进入新作用域（函数内部） */
        enter_scope();
        debug_content("  Entered scope level: ", get_current_scope_level());
        
        /* 添加参数到符号表 */
        ASTNode* param = node->data.subprog.params;
        int param_count = 0;
        
        debug_content("Processing params for function: ", node->data.subprog.name);
        
        /* 遍历所有参数组 */
        while (param&& param->type == AST_PARAM_LIST) {
            debug_content("param node type=",param->type," is_var=",param->data.param.is_var," type=",param->data.param.type);
            
            if (param->type == AST_PARAM_LIST) {
                debug_content("  Processing param group: is_var=%d, type=%d", 
                       param->data.param.is_var, param->data.param.type);
                
                /* 获取参数列表中的标识符 */
                ASTNode* id_list = param->data.param.id_list;
                while (id_list && id_list->type == AST_IDENTIFIER) {
                    debug_content("  Adding parameter: ",id_list->data.id_node.name ," type=", param->data.param.type);
                    
                    /* 添加参数作为变量 */
                    add_var(id_list->data.id_node.name, param->data.param.type, node->line);
                    param_count++;
                    
                    id_list = id_list->next;
                }
            } else {
                debug_content("WARNING: param node type is not AST_PARAM_LIST :", param->type);
            }
            
            param = param->data.param.next_param;
        }
        
        debug_content("  Total param_count for ",node->data.subprog.name," = ", param_count);
        
        /* 更新函数/过程的参数数量 */
        SymEntry* entry = lookup_symbol(node->data.subprog.name);
        if (entry && (entry->kind == SYM_FUNC || entry->kind == SYM_PROC)) {
            entry->u.func_info.param_count = param_count;
            debug_content(" Set param_count for ",node->data.subprog.name," to ", param_count);
        } else {
            debug_content("Failed to update param_count for function/procedure: ", node->data.subprog.name);
        }
        
        /* 处理局部变量声明 */
        process_var_decl(node->data.subprog.var_decls);
        
        /* 检查子程序体中的语句 */
        if (node->data.subprog.body) {
            check_statement(node->data.subprog.body);
        }
        
        /* 退出作用域 */
        exit_scope();
        debug_content("Exited scope, back to level: ", get_current_scope_level());
        
        /* 处理下一个子程序 */
        if (node->data.subprog.next_decl) {
            debug_content("Recursively processing next subprogram declaration");
            process_subprog_decl(node->data.subprog.next_decl);
        } else {
            debug_content("No more subprogram declarations");
        }
    } else {
        debug_content("WARNING: node type is not AST_SUBPROG_DECL (%d)", node->type);
    }
}

