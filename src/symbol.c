/**
 * @file symbol.c
 * @brief 符号表模块实现
 * @author 陈妍语
 */

#include "symbol.h"
#include "semantic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

SymTableManager sym_manager;

// 前向声明
static void add_symbol_to_current_scope(SymEntry* entry);
static void report_error(int line, const char* fmt, ...);

//====================================== 初始化/作用域函数 ======================================
/* 1. 初始化符号表 */
void init_symtable(void) {
    sym_manager.current_scope = NULL;
    sym_manager.scope_counter = 0;
    sym_manager.error_count = 0;
    
    /* 创建全局作用域 */
    enter_scope();
}

/* 2. 进入新作用域 */
void enter_scope(void) {
    Scope* new_scope = (Scope*)malloc(sizeof(Scope));
    new_scope->level = sym_manager.scope_counter++;
    new_scope->symbols = NULL;
    new_scope->parent = sym_manager.current_scope;
    new_scope->next = NULL;  // 初始化 next 为 NULL
    
    // 将新作用域添加到父作用域的 next 链表
    if (sym_manager.current_scope) {
        // 找到父作用域所在层级的最后一个兄弟
        Scope* sibling = sym_manager.current_scope;
        while (sibling->next) {
            sibling = sibling->next;
        }
        sibling->next = new_scope;  // 链接到兄弟链表
    }
    
    sym_manager.current_scope = new_scope;
}

/* 退出当前作用域 */
void exit_scope(void) {
    if (sym_manager.current_scope == NULL) return;
    
    // 只移动指针，不释放任何内存
    sym_manager.current_scope = sym_manager.current_scope->parent;
}

/* 获取当前作用域层级 */
int get_current_scope_level(void) {
    return sym_manager.current_scope ? sym_manager.current_scope->level : 0;
}




//====================================== 检查/查找函数 ======================================
/* 检查重复声明 */
int check_redeclaration(const char* name, int line) {
    SymEntry* existing = lookup_current_scope(name);
    if (existing) {
        fprintf(stderr, "[Semantic Error] Line %d: Redeclaration of '%s' (previously declared at line %d)\n",
                line, name, existing->line_declared);
        sym_manager.error_count++;
        return 1;
    }
    return 0;
}

/* 在当前作用域内查找（不向外） ，用于检查变量名是否重复。重复返回重复实体，不重复返回NULL*/
SymEntry* lookup_current_scope(const char* name) {
    if (sym_manager.current_scope == NULL) return NULL;
    
    SymEntry* entry = sym_manager.current_scope->symbols;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/* 从当前作用域向外查找 */
SymEntry* lookup_symbol(const char* name) {
    Scope* scope = sym_manager.current_scope;
    while (scope) {
        SymEntry* entry = scope->symbols;
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                return entry;
            }
            entry = entry->next;
        }
        scope = scope->parent;
    }
    return NULL;
}




//====================================== 添加到符号表函数 ======================================
/* 添加整型常量 */
SymEntry* add_const_int(const char* name, int value, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_CONST;
    entry->type = TYPE_INTEGER;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.const_value.int_val = value;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加实型常量 */
SymEntry* add_const_real(const char* name, double value, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_CONST;
    entry->type = TYPE_REAL;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.const_value.real_val = value;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加字符常量 */
SymEntry* add_const_char(const char* name, char value, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_CONST;
    entry->type = TYPE_CHAR;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.const_value.char_val = value;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加布尔常量 */
SymEntry* add_const_bool(const char* name, int value, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_CONST;
    entry->type = TYPE_BOOLEAN;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.const_value.bool_val = value;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加变量 */
SymEntry* add_var(const char* name, DataType type, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    debug_content("add_var: name=", name,"scope_level=", get_current_scope_level());
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_VAR;
    entry->type = type;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    
    add_symbol_to_current_scope(entry);
    
    return entry;
}

/* 添加数组 */
SymEntry* add_array(const char* name, ArrayInfo* info, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_VAR;
    entry->type = TYPE_ARRAY;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.array_info = *info;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加过程 */
SymEntry* add_proc(const char* name, ParamInfo* params, int param_count, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_PROC;
    entry->type = TYPE_UNKNOWN;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.func_info.params = params;
    entry->u.func_info.param_count = param_count;
    entry->u.func_info.has_return = 0;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加函数 */
SymEntry* add_func(const char* name, ParamInfo* params, int param_count, 
                    DataType return_type, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, PASCC_IDENT_LEN - 1);
    entry->name[PASCC_IDENT_LEN - 1] = '\0';
    entry->kind = SYM_FUNC;
    entry->type = return_type;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.func_info.params = params;
    entry->u.func_info.param_count = param_count;
    entry->u.func_info.return_type = return_type;
    entry->u.func_info.has_return = 0;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 在当前作用域添加符号（内部函数） */
static void add_symbol_to_current_scope(SymEntry* entry) {
    if (sym_manager.current_scope == NULL) return;
    
    entry->next = sym_manager.current_scope->symbols;
    sym_manager.current_scope->symbols = entry;
}



//====================================== 工具/辅助函数 ======================================
// 辅助函数
static const char* type_name(DataType t) {
    switch (t) {
        case TYPE_INTEGER: return "integer";
        case TYPE_REAL: return "real";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_CHAR: return "char";
        default: return "unknown";
    }
}
// 添加类型名称转换函数
static const char* data_type_to_string(DataType t) {
    switch (t) {
        case TYPE_INTEGER: return "integer";
        case TYPE_REAL: return "real";
        case TYPE_BOOLEAN: return "boolean";
        case TYPE_CHAR: return "char";
        case TYPE_ARRAY: return "array";
        default: return "unknown";
    }
}



//====================================== 打印符号表/报告错误函数 ======================================
/* 打印完整符号表（包含所有作用域） */
void print_complete_symtable(FILE* output) {
    if (output == NULL) output = stdout;
    
    fprintf(output, "\n");
    fprintf(output, "============================================================\n");
    fprintf(output, "                    COMPLETE SYMBOL TABLE\n");
    fprintf(output, "============================================================\n");
    
    // 找到根节点（全局作用域）
    Scope* root = sym_manager.current_scope;
    while (root && root->parent) {
        root = root->parent;
    }
    
    // 从根节点开始遍历所有作用域
    Scope* scope = root;
    int scope_index = 0;
    
    while (scope) {
        fprintf(output, "\n--- Scope%d (Level %d) ---\n", scope_index, scope->level);
        fprintf(output, "%-20s %-10s %-10s %s\n", "Name", "Kind", "Type", "Info");
        fprintf(output, "--------------------------------------------------------\n");
        
        SymEntry* entry = scope->symbols;
        if (!entry) {
            fprintf(output, "(empty)\n");
        }
        
        while (entry) {
            const char* kind_str = "";
            const char* type_str = "";
            
            switch (entry->kind) {
                case SYM_CONST: kind_str = "CONST"; break;
                case SYM_VAR: kind_str = "VAR"; break;
                case SYM_PROC: kind_str = "PROC"; break;
                case SYM_FUNC: kind_str = "FUNC"; break;
            }
            
            switch (entry->type) {
                case TYPE_INTEGER: type_str = "integer"; break;
                case TYPE_REAL: type_str = "real"; break;
                case TYPE_BOOLEAN: type_str = "boolean"; break;
                case TYPE_CHAR: type_str = "char"; break;
                case TYPE_ARRAY: type_str = "array"; break;
                default: type_str = "unknown"; break;
            }
            
            fprintf(output, "%-20s %-10s %-10s ", 
                    entry->name, kind_str, type_str);
                    
            if (entry->kind == SYM_CONST) {
                switch (entry->type) {
                    case TYPE_INTEGER:
                        fprintf(output, "= %d", entry->u.const_value.int_val);
                        break;
                    case TYPE_REAL:
                        fprintf(output, "= %f", entry->u.const_value.real_val);
                        break;
                    case TYPE_CHAR:
                        fprintf(output, "= '%c'", entry->u.const_value.char_val);
                        break;
                    case TYPE_BOOLEAN:
                        fprintf(output, "= %s", entry->u.const_value.bool_val ? "true" : "false");
                        break;
                    default:
                        fprintf(output, "= %d", entry->u.const_value.int_val);
                        break;
                }
            } else if (entry->type == TYPE_ARRAY) {
                fprintf(output, "[%d..%d] of %s", 
                        entry->u.array_info.low, 
                        entry->u.array_info.high,
                        type_name(entry->u.array_info.elem_type));
            } else if (entry->kind == SYM_FUNC) {
                fprintf(output, "returns %s, %d param(s)", 
                        type_name(entry->u.func_info.return_type),
                        entry->u.func_info.param_count);
            } else if (entry->kind == SYM_PROC) {
                fprintf(output, "%d param(s)", entry->u.func_info.param_count);
            }
            fprintf(output, "\n");
            
            entry = entry->next;
        }
        
        scope = scope->next;  // 使用 next 指针遍历兄弟作用域
        scope_index++;
    }
    
    fprintf(output, "\n============================================================\n");
    fprintf(output, "                    END OF SYMBOL TABLE\n");
    fprintf(output, "============================================================\n");
}

void print_symtable(FILE* output) {
    print_complete_symtable(output);
}

/* 报告语义错误 */
void report_error(int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[Symbol Table Error] Line %d: ", line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    sym_manager.error_count++;
}
/* 调试函数：用于输出手动调试的信息（支持可变参数，类似 printf） */
void debug_content(const char* fmt, ...) {
    if(g_verbose == 0) return;
    
    va_list args;
    va_start(args, fmt);
    
    // 使用动态缓冲区
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    printf("[Symbol Table Debug] %s", buffer);
    
    va_end(args);
}


//====================================== 释放资源函数 ======================================

/* 递归释放作用域树（包括所有子作用域和兄弟作用域） */
static void free_scope_tree(Scope* scope) {
    if (!scope) return;
    
    // 先释放所有子作用域（通过 next 链表）
    Scope* next_sibling = scope->next;
    while (next_sibling) {
        Scope* temp = next_sibling;
        next_sibling = next_sibling->next;
        
        // 递归释放子作用域的符号
        SymEntry* entry = temp->symbols;
        while (entry) {
            SymEntry* next_entry = entry->next;
            if ((entry->kind == SYM_FUNC || entry->kind == SYM_PROC) 
                && entry->u.func_info.params) {
                free(entry->u.func_info.params);
            }
            free(entry);
            entry = next_entry;
        }
        free(temp);
    }
    
    // 释放当前作用域的符号
    SymEntry* entry = scope->symbols;
    while (entry) {
        SymEntry* next_entry = entry->next;
        if ((entry->kind == SYM_FUNC || entry->kind == SYM_PROC) 
            && entry->u.func_info.params) {
            free(entry->u.func_info.params);
        }
        free(entry);
        entry = next_entry;
    }
    
    // 释放当前作用域
    free(scope);
}

void free_symtable(void) {
    // 找到根作用域（全局作用域）
    Scope* root = sym_manager.current_scope;
    while (root && root->parent) {
        root = root->parent;
    }
    
    // 从根作用域开始，遍历整个作用域树
    Scope* scope = root;
    while (scope) {
        Scope* next_parent = scope->parent;
        free_scope_tree(scope);
        scope = next_parent;
    }
    
    sym_manager.current_scope = NULL;
}