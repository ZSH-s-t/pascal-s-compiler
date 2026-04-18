@echo off
REM Build script for Pascal-S Compiler on Windows
REM Author: Team Members 4 & 5

echo ========================================
echo Building Pascal-S to C Compiler (pascc)
echo ========================================

REM Create build directory
if not exist build mkdir build

REM Check for flex
where flex >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: flex not found. Please install flex.
    exit /b 1
)

REM Check for gcc
where gcc >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: gcc not found. Please install MinGW or similar.
    exit /b 1
)

echo.
echo Step 1: Generating lexer with flex...
flex -o build\lex.yy.c src\lexer.l
if %ERRORLEVEL% NEQ 0 (
    echo Error: flex failed
    exit /b 1
)

echo Step 2: Compiling source files...
gcc -std=c99 -Wall -g -I./include -c -o build\token.o src\token.c
gcc -std=c99 -Wall -g -I./include -c -o build\error.o src\error.c
gcc -std=c99 -Wall -g -I./include -c -o build\ast.o src\ast.c
gcc -std=c99 -Wall -g -I./include -c -o build\parser.o src\parser.c
gcc -std=c99 -Wall -g -I./include -c -o build\semantic.o src\semantic.c
gcc -std=c99 -Wall -g -I./include -c -o build\symbol.o src\symbol.c
gcc -std=c99 -Wall -g -I./include -c -o build\codegen.o src\codegen.c
gcc -std=c99 -Wall -g -I./include -c -o build\main.o src\main.c
gcc -std=c99 -Wall -g -I./include -c -o build\lex.yy.o build\lex.yy.c

echo Step 3: Linking pascc...
gcc -std=c99 -Wall -g -I./include -o pascc.exe build\token.o build\error.o build\lex.yy.o build\ast.o build\parser.o build\semantic.o build\symbol.o build\codegen.o build\main.o

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build successful! pascc.exe created.
    echo ========================================
) else (
    echo.
    echo Build failed!
    exit /b 1
)
