/**
 * @file codegen.c
 * @brief 代码生成器实现
 * @author 组员4 & 组员5
 */

#include "codegen.h"
#include "symbol.h"
#include "error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 初始化代码生成器 */
void codegen_init(CodeGenContext *ctx, FILE *output) {
    ctx->output = output;
    ctx->indent_level = 0;
    ctx->temp_var_count = 0;
    ctx->label_count = 0;
    ctx->current_function = NULL;
}

/* 输出缩进 */
void codegen_indent(CodeGenContext *ctx) {
    for (int i = 0; i < ctx->indent_level; i++) {
        fprintf(ctx->output, "    ");
    }
}

/* 获取C语言类型名 */
const char* get_c_type(DataType type) {
    switch (type) {
        case TYPE_INTEGER: return "int";
        case TYPE_REAL: return "double";
        case TYPE_BOOLEAN: return "int";
        case TYPE_CHAR: return "char";
        default: return "int";
    }
}

/* 获取C语言运算符 */
const char* get_c_operator(BinaryOp op) {
    switch (op) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_DIV_INT: return "/";
        case OP_MOD: return "%";
        case OP_AND: return "&&";
        case OP_OR: return "||";
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_LT: return "<";
        case OP_LE: return "<=";
        case OP_GT: return ">";
        case OP_GE: return ">=";
        default: return "?";
    }
}

/* 前向声明 */
static void codegen_const_decls(CodeGenContext *ctx, ASTNode *node);
static void codegen_var_decls(CodeGenContext *ctx, ASTNode *node);
static void codegen_subprog_decls(CodeGenContext *ctx, ASTNode *node);
static void codegen_stmt_list(CodeGenContext *ctx, ASTNode *list);

/* 生成表达式 */
void codegen_expression(CodeGenContext *ctx, ASTNode *expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_CONST_VAL:
            switch (expr->data.const_val.token_type) {
                case TOKEN_INTEGER_CONST:
                    fprintf(ctx->output, "%d", expr->data.const_val.int_val);
                    break;
                case TOKEN_REAL_CONST:
                    fprintf(ctx->output, "%f", expr->data.const_val.real_val);
                    break;
                case TOKEN_CHAR_CONST:
                    fprintf(ctx->output, "'%c'", expr->data.const_val.char_val);
                    break;
                case TOKEN_TRUE:
                    fprintf(ctx->output, "1");
                    break;
                case TOKEN_FALSE:
                    fprintf(ctx->output, "0");
                    break;
                default:
                    break;
            }
            break;
            
        case AST_VAR_REF:
            /* 如果是当前函数名，添加_result后缀 */
            if (ctx->current_function && 
                strcmp(expr->data.var_ref.name, ctx->current_function) == 0) {
                fprintf(ctx->output, "%s_result", expr->data.var_ref.name);
            } else {
                fprintf(ctx->output, "%s", expr->data.var_ref.name);
            }
            if (expr->data.var_ref.index_expr) {
                fprintf(ctx->output, "[");
                codegen_expression(ctx, expr->data.var_ref.index_expr);
                fprintf(ctx->output, "]");
            }
            break;
            
        case AST_BINARY_EXPR:
            fprintf(ctx->output, "(");
            codegen_expression(ctx, expr->data.binary.left);
            fprintf(ctx->output, " %s ", get_c_operator(expr->data.binary.op));
            codegen_expression(ctx, expr->data.binary.right);
            fprintf(ctx->output, ")");
            break;
            
        case AST_UNARY_EXPR:
            if (expr->data.unary.op == UNARY_NOT) {
                fprintf(ctx->output, "!");
            } else if (expr->data.unary.op == UNARY_MINUS) {
                fprintf(ctx->output, "-");
            }
            fprintf(ctx->output, "(");
            codegen_expression(ctx, expr->data.unary.operand);
            fprintf(ctx->output, ")");
            break;
            
        case AST_CALL_EXPR:
            fprintf(ctx->output, "%s(", expr->data.call_expr.name);
            if (expr->data.call_expr.args) {
                ASTNode *args = expr->data.call_expr.args;
                if (args->type == AST_STMT_LIST) {
                    ASTNode *arg = args->data.stmt_list.first;
                    int first = 1;
                    while (arg) {
                        if (!first) fprintf(ctx->output, ", ");
                        codegen_expression(ctx, arg);
                        first = 0;
                        arg = arg->next;
                    }
                } else {
                    codegen_expression(ctx, args);
                }
            }
            fprintf(ctx->output, ")");
            break;
            
        default:
            break;
    }
}

/* 生成赋值语句 */
static void codegen_assign_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_indent(ctx);
    codegen_expression(ctx, node->data.assign.lhs);
    fprintf(ctx->output, " = ");
    codegen_expression(ctx, node->data.assign.rhs);
    fprintf(ctx->output, ";\n");
}

/* 生成if语句 */
static void codegen_if_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_indent(ctx);
    fprintf(ctx->output, "if (");
    codegen_expression(ctx, node->data.if_stmt.cond);
    fprintf(ctx->output, ") {\n");
    
    ctx->indent_level++;
    codegen_statement(ctx, node->data.if_stmt.then_part);
    ctx->indent_level--;
    
    if (node->data.if_stmt.else_part) {
        codegen_indent(ctx);
        fprintf(ctx->output, "} else {\n");
        ctx->indent_level++;
        codegen_statement(ctx, node->data.if_stmt.else_part);
        ctx->indent_level--;
    }
    
    codegen_indent(ctx);
    fprintf(ctx->output, "}\n");
}

/* 生成for语句 */
static void codegen_for_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_indent(ctx);
    fprintf(ctx->output, "for (%s = ", node->data.for_stmt.var_name);
    codegen_expression(ctx, node->data.for_stmt.start_expr);
    fprintf(ctx->output, "; %s <= ", node->data.for_stmt.var_name);
    codegen_expression(ctx, node->data.for_stmt.end_expr);
    fprintf(ctx->output, "; %s++) {\n", node->data.for_stmt.var_name);
    
    ctx->indent_level++;
    codegen_statement(ctx, node->data.for_stmt.body);
    ctx->indent_level--;
    
    codegen_indent(ctx);
    fprintf(ctx->output, "}\n");
}

/* 生成复合语句 */
static void codegen_compound_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_stmt_list(ctx, node->data.compound.stmt_list);
}

/* 获取表达式的类型（用于格式化输出） */
static DataType get_expr_type(ASTNode *expr) {
    if (!expr) return TYPE_INTEGER;
    
    switch (expr->type) {
        case AST_CONST_VAL:
            switch (expr->data.const_val.token_type) {
                case TOKEN_INTEGER_CONST: return TYPE_INTEGER;
                case TOKEN_REAL_CONST: return TYPE_REAL;
                case TOKEN_CHAR_CONST: return TYPE_CHAR;
                case TOKEN_TRUE:
                case TOKEN_FALSE: return TYPE_BOOLEAN;
                default: return TYPE_INTEGER;
            }
        case AST_VAR_REF: {
            SymEntry *sym = lookup_symbol(expr->data.var_ref.name);
            if (sym) {
                if (sym->type == TYPE_ARRAY) {
                    return sym->u.array_info.elem_type;
                }
                return sym->type;
            }
            return TYPE_INTEGER;
        }
        case AST_CALL_EXPR: {
            SymEntry *sym = lookup_symbol(expr->data.call_expr.name);
            if (sym && sym->kind == SYM_FUNC) {
                return sym->type;
            }
            return TYPE_INTEGER;
        }
        case AST_BINARY_EXPR:
            /* 关系运算符返回boolean */
            if (expr->data.binary.op >= OP_EQ && expr->data.binary.op <= OP_GE) {
                return TYPE_BOOLEAN;
            }
            /* 除法返回real */
            if (expr->data.binary.op == OP_DIV) {
                return TYPE_REAL;
            }
            /* 其他情况根据操作数类型 */
            {
                DataType left_type = get_expr_type(expr->data.binary.left);
                DataType right_type = get_expr_type(expr->data.binary.right);
                if (left_type == TYPE_REAL || right_type == TYPE_REAL) {
                    return TYPE_REAL;
                }
                return TYPE_INTEGER;
            }
        case AST_UNARY_EXPR:
            if (expr->data.unary.op == UNARY_NOT) {
                return TYPE_BOOLEAN;
            }
            return get_expr_type(expr->data.unary.operand);
        default:
            return TYPE_INTEGER;
    }
}

/* 生成read语句 */
static void codegen_read_stmt(CodeGenContext *ctx, ASTNode *node) {
    if (!node->data.read_stmt.var_list) return;
    
    ASTNode *var_list = node->data.read_stmt.var_list;
    if (var_list->type != AST_STMT_LIST) return;
    
    ASTNode *var = var_list->data.stmt_list.first;
    while (var) {
        codegen_indent(ctx);
        
        /* 根据变量类型生成不同的scanf格式 */
        if (var->type == AST_VAR_REF) {
            /* 查找变量类型 */
            SymEntry *sym = lookup_symbol(var->data.var_ref.name);
            if (sym) {
                DataType var_type = sym->type;
                if (var_type == TYPE_ARRAY) {
                    var_type = sym->u.array_info.elem_type;
                }
                
                const char *format = "";
                switch (var_type) {
                    case TYPE_INTEGER: format = "%d"; break;
                    case TYPE_REAL: format = "%lf"; break;
                    case TYPE_CHAR: format = " %c"; break;
                    default: format = "%d"; break;
                }
                
                fprintf(ctx->output, "scanf(\"%s\", &", format);
                codegen_expression(ctx, var);
                fprintf(ctx->output, ");\n");
            }
        }
        
        var = var->next;
    }
}

/* 生成write语句 */
static void codegen_write_stmt(CodeGenContext *ctx, ASTNode *node) {
    if (!node->data.write_stmt.expr_list) return;
    
    ASTNode *expr_list = node->data.write_stmt.expr_list;
    if (expr_list->type != AST_STMT_LIST) return;
    
    ASTNode *expr = expr_list->data.stmt_list.first;
    while (expr) {
        codegen_indent(ctx);
        
        /* 根据表达式类型生成不同的printf格式 */
        DataType expr_type = get_expr_type(expr);
        const char *format = "";
        switch (expr_type) {
            case TYPE_INTEGER: format = "%d"; break;
            case TYPE_REAL: format = "%f"; break;
            case TYPE_CHAR: format = "%c"; break;
            case TYPE_BOOLEAN: format = "%d"; break;
            default: format = "%d"; break;
        }
        
        fprintf(ctx->output, "printf(\"%s\", ", format);
        codegen_expression(ctx, expr);
        fprintf(ctx->output, ");\n");
        
        expr = expr->next;
    }
    
    /* 如果是writeln，添加换行 */
    if (node->data.write_stmt.is_writeln) {
        codegen_indent(ctx);
        fprintf(ctx->output, "printf(\"\\n\");\n");
    }
}

/* 生成语句 */
void codegen_statement(CodeGenContext *ctx, ASTNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_ASSIGN_STMT:
            codegen_assign_stmt(ctx, stmt);
            break;
            
        case AST_IF_STMT:
            codegen_if_stmt(ctx, stmt);
            break;
            
        case AST_FOR_STMT:
            codegen_for_stmt(ctx, stmt);
            break;
            
        case AST_COMPOUND_STMT:
            codegen_compound_stmt(ctx, stmt);
            break;
            
        case AST_READ_STMT:
            codegen_read_stmt(ctx, stmt);
            break;
            
        case AST_WRITE_STMT:
            codegen_write_stmt(ctx, stmt);
            break;
            
        case AST_CALL_STMT:
            codegen_indent(ctx);
            /* 特殊处理 writeln 和 write */
            if (strcmp(stmt->data.call_stmt.name, "writeln") == 0 || 
                strcmp(stmt->data.call_stmt.name, "write") == 0) {
                int is_writeln = (strcmp(stmt->data.call_stmt.name, "writeln") == 0);
                
                if (stmt->data.call_stmt.args) {
                    ASTNode *args = stmt->data.call_stmt.args;
                    if (args->type == AST_STMT_LIST) {
                        ASTNode *arg = args->data.stmt_list.first;
                        while (arg) {
                            DataType expr_type = get_expr_type(arg);
                            const char *format = "";
                            switch (expr_type) {
                                case TYPE_INTEGER: format = "%d"; break;
                                case TYPE_REAL: format = "%f"; break;
                                case TYPE_CHAR: format = "%c"; break;
                                case TYPE_BOOLEAN: format = "%d"; break;
                                default: format = "%d"; break;
                            }
                            fprintf(ctx->output, "printf(\"%s\", ", format);
                            codegen_expression(ctx, arg);
                            fprintf(ctx->output, ");\n");
                            arg = arg->next;
                            if (arg) codegen_indent(ctx);
                        }
                    } else {
                        DataType expr_type = get_expr_type(args);
                        const char *format = "";
                        switch (expr_type) {
                            case TYPE_INTEGER: format = "%d"; break;
                            case TYPE_REAL: format = "%f"; break;
                            case TYPE_CHAR: format = "%c"; break;
                            case TYPE_BOOLEAN: format = "%d"; break;
                            default: format = "%d"; break;
                        }
                        fprintf(ctx->output, "printf(\"%s\", ", format);
                        codegen_expression(ctx, args);
                        fprintf(ctx->output, ");\n");
                    }
                }
                
                if (is_writeln) {
                    codegen_indent(ctx);
                    fprintf(ctx->output, "printf(\"\\n\");\n");
                }
            } else {
                /* 普通过程调用 */
                fprintf(ctx->output, "%s(", stmt->data.call_stmt.name);
                if (stmt->data.call_stmt.args) {
                    ASTNode *args = stmt->data.call_stmt.args;
                    if (args->type == AST_STMT_LIST) {
                        ASTNode *arg = args->data.stmt_list.first;
                        int first = 1;
                        while (arg) {
                            if (!first) fprintf(ctx->output, ", ");
                            codegen_expression(ctx, arg);
                            first = 0;
                            arg = arg->next;
                        }
                    } else {
                        codegen_expression(ctx, args);
                    }
                }
                fprintf(ctx->output, ");\n");
            }
            break;
            
        default:
            break;
    }
}

/* 生成语句列表 */
static void codegen_stmt_list(CodeGenContext *ctx, ASTNode *list) {
    if (!list || list->type != AST_STMT_LIST) return;
    
    for (ASTNode *stmt = list->data.stmt_list.first; stmt; stmt = stmt->next) {
        codegen_statement(ctx, stmt);
    }
}

/* 生成常量声明 */
static void codegen_const_decls(CodeGenContext *ctx, ASTNode *node) {
    if (!node) return;
    
    while (node && node->type == AST_CONST_DECL) {
        codegen_indent(ctx);
        fprintf(ctx->output, "#define %s ", node->data.const_decl.name);
        if (node->data.const_decl.value) {
            codegen_expression(ctx, node->data.const_decl.value);
        }
        fprintf(ctx->output, "\n");
        node = node->data.const_decl.next_decl;
    }
}

/* 生成变量声明 */
static void codegen_var_decls(CodeGenContext *ctx, ASTNode *node) {
    if (!node) return;
    
    while (node && node->type == AST_VAR_DECL) {
        ASTNode *id = node->data.var_decl.id_list;
        while (id && id->type == AST_IDENTIFIER) {
            codegen_indent(ctx);
            fprintf(ctx->output, "%s %s", 
                    get_c_type(node->data.var_decl.data_type),
                    id->data.id_node.name);
            
            /* 处理数组 */
            if (node->data.var_decl.data_type == TYPE_ARRAY) {
                fprintf(ctx->output, "[100]");  /* 简化处理 */
            }
            
            fprintf(ctx->output, ";\n");
            id = id->next;
        }
        node = node->data.var_decl.next_decl;
    }
}

/* 生成函数/过程声明 */
static void codegen_subprog_decls(CodeGenContext *ctx, ASTNode *node) {
    if (!node) return;
    
    while (node && node->type == AST_SUBPROG_DECL) {
        fprintf(ctx->output, "\n");
        
        /* 函数返回类型 */
        if (node->data.subprog.is_function) {
            fprintf(ctx->output, "%s", get_c_type(node->data.subprog.return_type));
        } else {
            fprintf(ctx->output, "void");
        }
        
        /* 函数名 */
        fprintf(ctx->output, " %s(", node->data.subprog.name);
        
        /* 参数列表 */
        ASTNode *param = node->data.subprog.params;
        int first = 1;
        while (param && param->type == AST_PARAM_LIST) {
            ASTNode *id = param->data.param.id_list;
            while (id && id->type == AST_IDENTIFIER) {
                if (!first) fprintf(ctx->output, ", ");
                
                /* var参数用指针 */
                if (param->data.param.is_var) {
                    fprintf(ctx->output, "%s *%s", 
                            get_c_type(param->data.param.type),
                            id->data.id_node.name);
                } else {
                    fprintf(ctx->output, "%s %s", 
                            get_c_type(param->data.param.type),
                            id->data.id_node.name);
                }
                
                first = 0;
                id = id->next;
            }
            param = param->data.param.next_param;
        }
        
        if (first) {
            fprintf(ctx->output, "void");
        }
        
        fprintf(ctx->output, ") {\n");
        
        /* 函数体 */
        ctx->indent_level++;
        
        /* 设置当前函数名 */
        const char *prev_function = ctx->current_function;
        if (node->data.subprog.is_function) {
            ctx->current_function = node->data.subprog.name;
        }
        
        /* 如果是函数，声明返回值变量 */
        if (node->data.subprog.is_function) {
            codegen_indent(ctx);
            fprintf(ctx->output, "%s %s_result;\n", 
                    get_c_type(node->data.subprog.return_type),
                    node->data.subprog.name);
        }
        
        codegen_statement(ctx, node->data.subprog.body);
        
        /* 如果是函数，添加return语句 */
        if (node->data.subprog.is_function) {
            codegen_indent(ctx);
            fprintf(ctx->output, "return %s_result;\n", node->data.subprog.name);
        }
        
        /* 恢复之前的函数名 */
        ctx->current_function = prev_function;
        
        ctx->indent_level--;
        fprintf(ctx->output, "}\n");
        
        node = node->data.subprog.next_decl;
    }
}

/* 生成声明部分 */
void codegen_declarations(CodeGenContext *ctx, ASTNode *const_decls, 
                          ASTNode *var_decls, ASTNode *subprog_decls) {
    /* 生成常量定义 */
    if (const_decls) {
        codegen_const_decls(ctx, const_decls);
        fprintf(ctx->output, "\n");
    }
    
    /* 生成函数/过程声明 */
    if (subprog_decls) {
        codegen_subprog_decls(ctx, subprog_decls);
    }
}

/* 生成完整的C程序 */
int codegen_program(ASTNode *ast, FILE *output) {
    if (!ast || ast->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid AST for code generation\n");
        return -1;
    }
    
    CodeGenContext ctx;
    codegen_init(&ctx, output);
    
    /* 生成头文件包含 */
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <stdlib.h>\n");
    fprintf(output, "#include <math.h>\n\n");
    
    /* 生成常量和子程序声明 */
    codegen_declarations(&ctx, 
                        ast->data.program.const_decls,
                        NULL,
                        ast->data.program.subprog_decls);
    
    /* 生成main函数 */
    fprintf(output, "\nint main(void) {\n");
    ctx.indent_level = 1;
    
    /* 生成变量声明 */
    codegen_var_decls(&ctx, ast->data.program.var_decls);
    
    if (ast->data.program.var_decls) {
        fprintf(output, "\n");
    }
    
    /* 生成主程序体 */
    codegen_statement(&ctx, ast->data.program.body);
    
    /* 生成return语句 */
    codegen_indent(&ctx);
    fprintf(output, "return 0;\n");
    
    fprintf(output, "}\n");
    
    return 0;
}
