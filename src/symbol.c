/**
 * @file symbol.c
 * @brief 符号表模块实现
 * @author 陈妍语
 */

#include "symbol.h"
#include <stdlib.h>
#include <string.h>

SymTableManager sym_manager;

/* 初始化符号表 */
void init_symtable(void) {
    sym_manager.current_scope = NULL;
    sym_manager.scope_counter = 0;
    sym_manager.error_count = 0;
    
    /* 创建全局作用域 */
    enter_scope();
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

/* 进入新作用域 */
void enter_scope(void) {
    Scope* new_scope = (Scope*)malloc(sizeof(Scope));
    new_scope->level = sym_manager.scope_counter++;
    new_scope->symbols = NULL;
    new_scope->parent = sym_manager.current_scope;
    sym_manager.current_scope = new_scope;
}

void exit_scope(void) {
    if (sym_manager.current_scope == NULL) return;
    
    // 只移动指针，不释放任何内存
    sym_manager.current_scope = sym_manager.current_scope->parent;
}

/* 获取当前作用域层级 */
int get_current_scope_level(void) {
    return sym_manager.current_scope ? sym_manager.current_scope->level : 0;
}

/* 在当前作用域添加符号（内部函数） */
static void add_symbol_to_current_scope(SymEntry* entry) {
    if (sym_manager.current_scope == NULL) return;
    
    entry->next = sym_manager.current_scope->symbols;
    sym_manager.current_scope->symbols = entry;
}

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

/* 添加常量 */
SymEntry* add_const(const char* name, int value, DataType type, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, 127);
    entry->kind = SYM_CONST;
    entry->type = type;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    entry->u.const_value = value;
    
    add_symbol_to_current_scope(entry);
    return entry;
}

/* 添加变量 */
SymEntry* add_var(const char* name, DataType type, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    //printf("[DEBUG add_var] name=%s, scope_level=%d\n", name, get_current_scope_level());
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, 127);
    entry->kind = SYM_VAR;
    entry->type = type;
    entry->scope_level = get_current_scope_level();
    entry->line_declared = line;
    
    add_symbol_to_current_scope(entry);
    
    //printf("[DEBUG add_var] added %s, current_scope symbols: %p\n", name, sym_manager.current_scope->symbols);
    
    return entry;
}

/* 添加数组 */
SymEntry* add_array(const char* name, ArrayInfo* info, int line) {
    if (check_redeclaration(name, line)) return NULL;
    
    SymEntry* entry = (SymEntry*)calloc(1, sizeof(SymEntry));
    strncpy(entry->name, name, 127);
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
    strncpy(entry->name, name, 127);
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
    strncpy(entry->name, name, 127);
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

/* 在当前作用域内查找（不向外） */
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



/* 打印完整符号表（包含所有作用域） */
void print_complete_symtable(FILE* output) {
    if (output == NULL) output = stdout;
    
    fprintf(output, "\n");
    fprintf(output, "============================================================\n");
    fprintf(output, "                    COMPLETE SYMBOL TABLE\n");
    fprintf(output, "============================================================\n");
    
    Scope* root = sym_manager.current_scope;
    while (root && root->parent) {
        root = root->parent;
    }
    
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
                fprintf(output, "= %d", entry->u.const_value);
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
        
        scope = scope->parent;
        scope_index++;
    }
    
    fprintf(output, "\n============================================================\n");
    fprintf(output, "                    END OF SYMBOL TABLE\n");
    fprintf(output, "============================================================\n");
}

void print_symtable(FILE* output) {
    print_complete_symtable(output);
}



void free_symtable(void) {
    // 找到根作用域
    Scope* root = sym_manager.current_scope;
    while (root && root->parent) {
        root = root->parent;
    }
    
    // 遍历所有作用域并释放
    Scope* scope = root;
    while (scope) {
        SymEntry* entry = scope->symbols;
        while (entry) {
            SymEntry* next = entry->next;
            if ((entry->kind == SYM_FUNC || entry->kind == SYM_PROC) 
                && entry->u.func_info.params) {
                free(entry->u.func_info.params);
            }
            free(entry);
            entry = next;
        }
        Scope* next_scope = scope->parent;
        free(scope);
        scope = next_scope;
    }
    
    sym_manager.current_scope = NULL;
}