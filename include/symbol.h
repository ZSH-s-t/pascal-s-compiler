/**
 * @file symbol.h
 * @brief 符号表模块头文件
 * @author 陈妍语
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"
#include <stdio.h>

/* 全局变量：verbose 模式 */
extern int g_verbose;

/* 符号类型 */
typedef enum {
    SYM_CONST,      // 常量
    SYM_VAR,        // 变量
    SYM_PROC,       // 过程
    SYM_FUNC        // 函数
} SymKind;

/* 数组信息 */
typedef struct {
    int low;            // 下界
    int high;           // 上界
    DataType elem_type; // 元素类型
} ArrayInfo;

/* 参数信息 */
typedef struct {
    char name[64];
    DataType type;
    int is_var;         // 是否为var参数（引用传递）
} ParamInfo;

/* 函数/过程信息 */
typedef struct {
    ParamInfo* params;      // 参数列表（动态分配）
    int param_count;
    DataType return_type;   // 仅函数有效
    int has_return;         // 函数是否有返回值赋值
} FuncInfo;

/* 符号表条目 */
typedef struct SymEntry {
    char name[128];
    SymKind kind;
    DataType type;
    int scope_level;
    int line_declared;
    
    union {
        union {
            int int_val;
            double real_val;
            char char_val;
            int bool_val;
        } const_value;      // 修改为嵌套 union
        ArrayInfo array_info;
        FuncInfo func_info;
    } u;
    
    struct SymEntry* next;
    struct SymEntry* scope_next;
} SymEntry;

/* 作用域 */
typedef struct Scope {
    int level;// 作用域层级
    SymEntry* symbols; // 该作用域的符号链表
    struct Scope* parent;// 父作用域
    struct Scope* next;  // 下一个作用域（兄弟节点）
} Scope;

/* 符号表管理器 */
typedef struct {
    Scope* current_scope;// 当前作用域，是一个链表
    int scope_counter;// 作用域计数器
    int error_count;  // 错误计数器
} SymTableManager;

/* 全局符号表管理器（供各模块使用） */
extern SymTableManager sym_manager;

/* 初始化符号表 */
void init_symtable(void);

/* 作用域管理 */
void enter_scope(void);
void exit_scope(void);
int get_current_scope_level(void);

/* 符号添加 */
SymEntry* add_const_int(const char* name, int value, int line);
SymEntry* add_const_real(const char* name, double value, int line);
SymEntry* add_const_char(const char* name, char value, int line);
SymEntry* add_const_bool(const char* name, int value, int line);
SymEntry* add_var(const char* name, DataType type, int line);
SymEntry* add_array(const char* name, ArrayInfo* info, int line);
SymEntry* add_proc(const char* name, ParamInfo* params, int param_count, int line);
SymEntry* add_func(const char* name, ParamInfo* params, int param_count, 
                    DataType return_type, int line);

/* 符号查找（从当前作用域向外） */
SymEntry* lookup_symbol(const char* name);

/* 当前作用域内查找（不向外） */
SymEntry* lookup_current_scope(const char* name);

/* 检查重复声明 */
int check_redeclaration(const char* name, int line);

/* 打印符号表（调试用） */
void print_symtable(FILE* output);

/* 打印完整符号表（包含所有作用域） */
void print_complete_symtable(FILE* output);

/* 释放符号表 */
void free_symtable(void);

#endif