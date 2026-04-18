#!/bin/bash
# 头歌平台构建脚本
# 编译 Pascal-S 到 C 翻译器，输出到 /data/workspace/myshixun/bin/pascc

set -e

BIN_DIR="/data/workspace/myshixun/bin"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

cd "$SCRIPT_DIR"

# 创建 bin 目录
mkdir -p "$BIN_DIR"

# 创建 build 目录
mkdir -p build

# 用 flex 生成词法分析器
flex -o build/lex.yy.c src/lexer.l

# 编译各模块
gcc -std=c99 -Wall -I./include -c -o build/token.o    src/token.c
gcc -std=c99 -Wall -I./include -c -o build/error.o    src/error.c
gcc -std=c99 -Wall -I./include -c -o build/ast.o      src/ast.c
gcc -std=c99 -Wall -I./include -c -o build/parser.o   src/parser.c
gcc -std=c99 -Wall -I./include -c -o build/semantic.o src/semantic.c
gcc -std=c99 -Wall -I./include -c -o build/symbol.o   src/symbol.c
gcc -std=c99 -Wall -I./include -c -o build/codegen.o  src/codegen.c
gcc -std=c99 -Wall -I./include -c -o build/main.o     src/main.c
gcc -std=c99 -Wall -I./include -c -o build/lex.yy.o   build/lex.yy.c

# 链接生成 pascc
gcc -std=c99 -o "$BIN_DIR/pascc" \
    build/token.o build/error.o build/lex.yy.o \
    build/ast.o build/parser.o build/semantic.o \
    build/symbol.o build/codegen.o build/main.o \
    -lfl

echo "Build success: $BIN_DIR/pascc"
