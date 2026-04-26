#!/bin/bash
# Build script for Pascal-S Compiler on Linux
# Author: Team Members 4 & 5

echo "========================================"
echo "Building Pascal-S to C Compiler (pascc)"
echo "========================================"

# Create build directory
if [ ! -d build ]; then
    mkdir build
fi

# Check for flex
if ! command -v flex &> /dev/null; then
    echo "Error: flex not found. Please install flex."
    exit 1
fi

# Check for gcc
if ! command -v gcc &> /dev/null; then
    echo "Error: gcc not found. Please install gcc."
    exit 1
fi

echo ""
echo "Step 1: Generating lexer with flex..."
flex -o build/lex.yy.c src/lexer.l
if [ $? -ne 0 ]; then
    echo "Error: flex failed"
    exit 1
fi

echo "Step 2: Compiling source files..."
gcc -std=c99 -Wall -g -I./include -c -o build/token.o src/token.c
gcc -std=c99 -Wall -g -I./include -c -o build/error.o src/error.c
gcc -std=c99 -Wall -g -I./include -c -o build/ast.o src/ast.c
gcc -std=c99 -Wall -g -I./include -c -o build/parser.o src/parser.c
gcc -std=c99 -Wall -g -I./include -c -o build/semantic.o src/semantic.c
gcc -std=c99 -Wall -g -I./include -c -o build/symbol.o src/symbol.c
gcc -std=c99 -Wall -g -I./include -c -o build/codegen.o src/codegen.c
gcc -std=c99 -Wall -g -I./include -c -o build/main.o src/main.c
gcc -std=c99 -Wall -g -I./include -c -o build/lex.yy.o build/lex.yy.c

echo "Step 3: Linking pascc..."
gcc -std=c99 -Wall -g -I./include -o pascc build/token.o build/error.o build/lex.yy.o build/ast.o build/parser.o build/semantic.o build/symbol.o build/codegen.o build/main.o

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "Build successful! pascc created."
    echo "========================================"
else
    echo ""
    echo "Build failed!"
    exit 1
fi
