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
    ctx->current_subprog = NULL;
    ctx->subprog_decl_list = NULL;
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
static DataType get_expr_type(const CodeGenContext *ctx, ASTNode *expr);

/* 语义分析后已 exit_scope，lookup_symbol 找不到子程序内局部量；按当前子程序 AST 回退查找 */
typedef struct {
    int found;
    DataType type;
    const char *cname; /* 声明中的拼写，供 C 输出 */
} PasDeclInfo;

static PasDeclInfo lookup_decl_in_subprog(const ASTNode *sub, const char *name) {
    PasDeclInfo d = {0, TYPE_UNKNOWN, NULL};
    if (!name || !sub || sub->type != AST_SUBPROG_DECL)
        return d;
    ASTNode *param = sub->data.subprog.params;
    while (param && param->type == AST_PARAM_LIST) {
        ASTNode *id = param->data.param.id_list;
        while (id && id->type == AST_IDENTIFIER) {
            if (pascc_ident_equal(id->data.id_node.name, name) == 0) {
                d.found = 1;
                d.type = param->data.param.type;
                d.cname = id->data.id_node.name;
                return d;
            }
            id = id->next;
        }
        param = param->data.param.next_param;
    }
        for (ASTNode *vd = sub->data.subprog.var_decls; vd && vd->type == AST_VAR_DECL;
         vd = vd->data.var_decl.next_decl) {
        ASTNode *id = vd->data.var_decl.id_list;
        while (id && id->type == AST_IDENTIFIER) {
            if (pascc_ident_equal(id->data.id_node.name, name) == 0) {
                d.found = 1;
                d.type = vd->data.var_decl.data_type;
                if (d.type == TYPE_ARRAY)
                    d.type = vd->data.var_decl.elem_type;
                d.cname = id->data.id_node.name;
                return d;
            }
            id = id->next;
        }
    }
    return d;
}

static ASTNode *find_subprog_decl(ASTNode *head, const char *name) {
    ASTNode *n = head;
    while (n && n->type == AST_SUBPROG_DECL) {
        if (pascc_ident_equal(n->data.subprog.name, name) == 0)
            return n;
        n = n->data.subprog.next_decl;
    }
    return NULL;
}

static int is_var_formal_param(const CodeGenContext *ctx, const char *name) {
    const ASTNode *sub = ctx->current_subprog;
    if (!name || !sub || sub->type != AST_SUBPROG_DECL)
        return 0;
    ASTNode *param = sub->data.subprog.params;
    while (param && param->type == AST_PARAM_LIST) {
        if (param->data.param.is_var) {
            ASTNode *id = param->data.param.id_list;
            while (id && id->type == AST_IDENTIFIER) {
                if (pascc_ident_equal(id->data.id_node.name, name) == 0)
                    return 1;
                id = id->next;
            }
        }
        param = param->data.param.next_param;
    }
    return 0;
}

/* var 形参对应的实参：已是指针则传名，否则传 &lvalue */
static void codegen_actual_for_var_formal(CodeGenContext *ctx, ASTNode *arg) {
    if (arg && arg->type == AST_VAR_REF) {
        const char *v = arg->data.var_ref.name;
        if (arg->data.var_ref.index_expr) {
            fprintf(ctx->output, "&(");
            codegen_expression(ctx, arg);
            fprintf(ctx->output, ")");
            return;
        }
        {
            SymEntry *vs = lookup_symbol(v);
            const char *cn = (vs && vs->name[0]) ? vs->name : v;
            if (!vs && ctx->current_subprog) {
                PasDeclInfo di = lookup_decl_in_subprog(ctx->current_subprog, v);
                if (di.found && di.cname)
                    cn = di.cname;
            }
            if (is_var_formal_param(ctx, v))
                fprintf(ctx->output, "%s", cn);
            else
                fprintf(ctx->output, "&%s", cn);
        }
        return;
    }
    fprintf(ctx->output, "&(");
    codegen_expression(ctx, arg);
    fprintf(ctx->output, ")");
}

typedef struct {
    ASTNode *param;
    ASTNode *id;
} FormalWalk;

static void formal_walk_init(FormalWalk *w, ASTNode *param_chain) {
    w->param = param_chain;
    w->id = (param_chain && param_chain->type == AST_PARAM_LIST)
                ? param_chain->data.param.id_list
                : NULL;
}

static int formal_walk_is_var(const FormalWalk *w) {
    return w->param && w->param->type == AST_PARAM_LIST && w->param->data.param.is_var;
}

static void formal_walk_advance(FormalWalk *w) {
    if (!w->param) return;
    if (w->id)
        w->id = w->id->next;
    if (!w->id) {
        w->param = w->param->data.param.next_param;
        w->id = (w->param && w->param->type == AST_PARAM_LIST)
                    ? w->param->data.param.id_list
                    : NULL;
    }
}

static void codegen_call_args_with_params(CodeGenContext *ctx, ASTNode *args,
                                          ASTNode *param_chain) {
    FormalWalk fw;
    formal_walk_init(&fw, param_chain);
    int first = 1;
    if (!args)
        return;
    if (args->type == AST_STMT_LIST) {
        ASTNode *arg = args->data.stmt_list.first;
        while (arg) {
            if (!first)
                fprintf(ctx->output, ", ");
            if (formal_walk_is_var(&fw))
                codegen_actual_for_var_formal(ctx, arg);
            else
                codegen_expression(ctx, arg);
            first = 0;
            formal_walk_advance(&fw);
            arg = arg->next;
        }
    } else {
        if (formal_walk_is_var(&fw))
            codegen_actual_for_var_formal(ctx, args);
        else
            codegen_expression(ctx, args);
        formal_walk_advance(&fw);
    }
}

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
                    fprintf(ctx->output, "%.6f", expr->data.const_val.real_val);
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
            
        case AST_VAR_REF: {
            const char *vname = expr->data.var_ref.name;
            SymEntry *sym = lookup_symbol(vname);
            const char *cid = (sym && sym->name[0]) ? sym->name : vname;
            if (!sym && ctx->current_subprog) {
                PasDeclInfo di = lookup_decl_in_subprog(ctx->current_subprog, vname);
                if (di.found && di.cname)
                    cid = di.cname;
            }
            /* 函数体内对函数名赋值：指向隐式返回值变量 */
            if (ctx->current_function && ctx->current_subprog &&
                ctx->current_subprog->type == AST_SUBPROG_DECL &&
                pascc_ident_equal(vname, ctx->current_subprog->data.subprog.name) == 0) {
                fprintf(ctx->output, "%s_result",
                        ctx->current_subprog->data.subprog.name);
            } else if (is_var_formal_param(ctx, vname)) {
                fprintf(ctx->output, "(*%s)", cid);
            } else {
                fprintf(ctx->output, "%s", cid);
                /* 无参函数在表达式中必须按调用生成，否则在 C 中会退化为函数指针 */
                if (sym && sym->kind == SYM_FUNC &&
                    sym->u.func_info.param_count == 0) {
                    fprintf(ctx->output, "()");
                }
            }
            if (expr->data.var_ref.index_expr) {
                fprintf(ctx->output, "[");
                /* 处理多维数组下标 */
                if (expr->data.var_ref.index_expr->type == AST_STMT_LIST) {
                    /* 多维数组：a[i, j, k] -> a[i][j][k] */
                    ASTNode *idx = expr->data.var_ref.index_expr->data.stmt_list.first;
                    int first = 1;
                    while (idx) {
                        if (!first) fprintf(ctx->output, "][");
                        codegen_expression(ctx, idx);
                        first = 0;
                        idx = idx->next;
                    }
                } else {
                    codegen_expression(ctx, expr->data.var_ref.index_expr);
                }
                fprintf(ctx->output, "]");
            }
            break;
        }
            
        case AST_BINARY_EXPR:
            fprintf(ctx->output, "(");
            codegen_expression(ctx, expr->data.binary.left);
            fprintf(ctx->output, " %s ", get_c_operator(expr->data.binary.op));
            codegen_expression(ctx, expr->data.binary.right);
            fprintf(ctx->output, ")");
            break;
            
        case AST_UNARY_EXPR:
            if (expr->data.unary.op == UNARY_NOT) {
                /* 整数/非布尔：Pascal not 为按位取反；布尔：逻辑非 */
                if (get_expr_type(ctx, expr->data.unary.operand) == TYPE_BOOLEAN) {
                    fprintf(ctx->output, "!");
                } else {
                    fprintf(ctx->output, "~");
                }
            } else if (expr->data.unary.op == UNARY_MINUS) {
                fprintf(ctx->output, "-");
            }
            fprintf(ctx->output, "(");
            codegen_expression(ctx, expr->data.unary.operand);
            fprintf(ctx->output, ")");
            break;
            
        case AST_CALL_EXPR: {
            ASTNode *callee = find_subprog_decl(ctx->subprog_decl_list,
                                                  expr->data.call_expr.name);
            const char *callee_cname = callee ? callee->data.subprog.name
                                               : expr->data.call_expr.name;
            fprintf(ctx->output, "%s(", callee_cname);
            if (callee && callee->type == AST_SUBPROG_DECL) {
                codegen_call_args_with_params(ctx, expr->data.call_expr.args,
                                              callee->data.subprog.params);
            } else if (expr->data.call_expr.args) {
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
        }
            
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

/* 生成 while 语句 */
static void codegen_while_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_indent(ctx);
    fprintf(ctx->output, "while (");
    codegen_expression(ctx, node->data.while_stmt.cond);
    fprintf(ctx->output, ") {\n");
    ctx->indent_level++;
    codegen_statement(ctx, node->data.while_stmt.body);
    ctx->indent_level--;
    codegen_indent(ctx);
    fprintf(ctx->output, "}\n");
}

/* 生成复合语句 */
static void codegen_compound_stmt(CodeGenContext *ctx, ASTNode *node) {
    codegen_stmt_list(ctx, node->data.compound.stmt_list);
}

/* 获取表达式的类型（用于格式化输出） */
static DataType get_expr_type(const CodeGenContext *ctx, ASTNode *expr) {
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
            if (ctx && ctx->current_subprog) {
                PasDeclInfo di = lookup_decl_in_subprog(ctx->current_subprog,
                                                         expr->data.var_ref.name);
                if (di.found)
                    return di.type;
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
                DataType left_type = get_expr_type(ctx, expr->data.binary.left);
                DataType right_type = get_expr_type(ctx, expr->data.binary.right);
                if (left_type == TYPE_REAL || right_type == TYPE_REAL) {
                    return TYPE_REAL;
                }
                return TYPE_INTEGER;
            }
        case AST_UNARY_EXPR:
            if (expr->data.unary.op == UNARY_NOT) {
                if (get_expr_type(ctx, expr->data.unary.operand) == TYPE_BOOLEAN) {
                    return TYPE_BOOLEAN;
                }
                return TYPE_INTEGER;
            }
            return get_expr_type(ctx, expr->data.unary.operand);
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
            SymEntry *sym = lookup_symbol(var->data.var_ref.name);
            PasDeclInfo di = {0, TYPE_UNKNOWN, NULL};
            if (!sym && ctx->current_subprog)
                di = lookup_decl_in_subprog(ctx->current_subprog, var->data.var_ref.name);

            if (sym || di.found) {
                DataType var_type;
                const char *vname_c;

                if (sym) {
                    var_type = sym->type;
                    if (var_type == TYPE_ARRAY)
                        var_type = sym->u.array_info.elem_type;
                    vname_c = (sym->name[0]) ? sym->name : var->data.var_ref.name;
                } else {
                    var_type = di.type;
                    vname_c = di.cname ? di.cname : var->data.var_ref.name;
                }

                const char *format = "";
                switch (var_type) {
                    case TYPE_INTEGER: format = "%d"; break;
                    case TYPE_REAL: format = "%lf"; break;
                    case TYPE_CHAR: format = " %c"; break;
                    default: format = "%d"; break;
                }

                if (!var->data.var_ref.index_expr && ctx->current_function &&
                    ctx->current_subprog &&
                    ctx->current_subprog->type == AST_SUBPROG_DECL &&
                    ctx->current_subprog->data.subprog.is_function &&
                    pascc_ident_equal(var->data.var_ref.name, ctx->current_function) == 0) {
                    fprintf(ctx->output, "scanf(\"%s\", &%s_result);\n", format,
                            ctx->current_subprog->data.subprog.name);
                } else {
                    fprintf(ctx->output, "scanf(\"%s\", ", format);
                    if (is_var_formal_param(ctx, var->data.var_ref.name) &&
                        !var->data.var_ref.index_expr) {
                        fprintf(ctx->output, "%s", vname_c);
                    } else if (var->data.var_ref.index_expr) {
                        fprintf(ctx->output, "&(");
                        codegen_expression(ctx, var);
                        fprintf(ctx->output, ")");
                    } else {
                        fprintf(ctx->output, "&%s", vname_c);
                    }
                    fprintf(ctx->output, ");\n");
                }
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
        DataType expr_type = get_expr_type(ctx, expr);
        const char *format = "";
        switch (expr_type) {
            case TYPE_INTEGER: format = "%d"; break;
            case TYPE_REAL: format = "%.6f"; break;
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

        case AST_WHILE_STMT:
            codegen_while_stmt(ctx, stmt);
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
                            DataType expr_type = get_expr_type(ctx, arg);
                            const char *format = "";
                            switch (expr_type) {
                                case TYPE_INTEGER: format = "%d"; break;
                                case TYPE_REAL: format = "%.6f"; break;
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
                        DataType expr_type = get_expr_type(ctx, args);
                        const char *format = "";
                        switch (expr_type) {
                            case TYPE_INTEGER: format = "%d"; break;
                            case TYPE_REAL: format = "%.6f"; break;
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
                ASTNode *callee = find_subprog_decl(ctx->subprog_decl_list,
                                                    stmt->data.call_stmt.name);
                const char *cname = callee ? callee->data.subprog.name
                                           : stmt->data.call_stmt.name;
                fprintf(ctx->output, "%s(", cname);
                if (callee && callee->type == AST_SUBPROG_DECL) {
                    codegen_call_args_with_params(ctx, stmt->data.call_stmt.args,
                                                  callee->data.subprog.params);
                } else if (stmt->data.call_stmt.args) {
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
            
            /* 处理数组 */
            if (node->data.var_decl.data_type == TYPE_ARRAY) {
                fprintf(ctx->output, "%s %s", 
                        get_c_type(node->data.var_decl.elem_type),
                        id->data.id_node.name);
                /* 处理数组边界（支持多维） */
                if (node->data.var_decl.array_bounds &&
                    node->data.var_decl.array_bounds->type == AST_STMT_LIST) {
                    ASTNode *bound = node->data.var_decl.array_bounds->data.stmt_list.first;
                    while (bound) {
                        ASTNode *second = bound->next;
                        if (bound && bound->type == AST_CONST_VAL &&
                            second && second->type == AST_CONST_VAL) {
                            int size = second->data.const_val.int_val - bound->data.const_val.int_val + 1;
                            fprintf(ctx->output, "[%d]", size);
                        }
                        bound = second->next;  /* 跳到下一维 */
                    }
                } else {
                    fprintf(ctx->output, "[100]");  /* 默认大小 */
                }
            } else {
                fprintf(ctx->output, "%s %s", 
                        get_c_type(node->data.var_decl.data_type),
                        id->data.id_node.name);
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
        
        const ASTNode *prev_sub = ctx->current_subprog;
        ctx->current_subprog = node;
        
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
        
        /* 生成局部变量声明 */
        codegen_var_decls(ctx, node->data.subprog.var_decls);
        
        codegen_statement(ctx, node->data.subprog.body);
        
        /* 如果是函数，添加return语句 */
        if (node->data.subprog.is_function) {
            codegen_indent(ctx);
            fprintf(ctx->output, "return %s_result;\n", node->data.subprog.name);
        }
        
        /* 恢复之前的函数名与 subprog 上下文 */
        ctx->current_function = prev_function;
        ctx->current_subprog = prev_sub;
        
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
    ctx.subprog_decl_list = ast->data.program.subprog_decls;
    
    /* 生成头文件包含 */
    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <stdlib.h>\n");
    fprintf(output, "#include <math.h>\n\n");
    
    /* 常量 */
    if (ast->data.program.const_decls) {
        codegen_const_decls(&ctx, ast->data.program.const_decls);
        fprintf(output, "\n");
    }
    
    /* Pascal 程序级 var 对应 C 文件作用域全局变量，供子程序/函数访问 */
    ctx.indent_level = 0;
    codegen_var_decls(&ctx, ast->data.program.var_decls);
    if (ast->data.program.var_decls) {
        fprintf(output, "\n");
    }
    
    /* 子程序（可读写上述全局量） */
    if (ast->data.program.subprog_decls) {
        codegen_subprog_decls(&ctx, ast->data.program.subprog_decls);
    }
    
    /* 生成main函数（程序体不再重复声明程序级变量） */
    fprintf(output, "\nint main(void) {\n");
    ctx.indent_level = 1;
    
    /* 生成主程序体 */
    codegen_statement(&ctx, ast->data.program.body);
    
    /* 生成return语句 */
    codegen_indent(&ctx);
    fprintf(output, "return 0;\n");
    
    fprintf(output, "}\n");
    
    return 0;
}
