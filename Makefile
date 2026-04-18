# Makefile for Pascal-S Lexer & Parser
# 作者: 组员1 - 词法分析器开发
# 扩展: 组员2 - 语法分析器开发

# 编译器和工具
CC = gcc
LEX = flex
CFLAGS = -Wall -g -I./include
LDFLAGS = -lfl

# 目录
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = test

# ========== 词法分析器源文件 ==========
LEX_SRC = $(SRC_DIR)/lexer.l
LEXER_C_SRCS = $(SRC_DIR)/token.c $(SRC_DIR)/error.c $(SRC_DIR)/main.c

# ========== 语法分析器新增源文件 ==========
PARSER_C_SRCS = $(SRC_DIR)/ast.c $(SRC_DIR)/parser.c
PARSER_TEST_SRC = $(SRC_DIR)/parser_test.c

# ========== 语义分析器和代码生成器源文件 ==========
SEMANTIC_C_SRCS = $(SRC_DIR)/semantic.c $(SRC_DIR)/symbol.c
CODEGEN_C_SRCS = $(SRC_DIR)/codegen.c

# ========== 生成的文件 ==========
LEX_GEN = $(BUILD_DIR)/lex.yy.c

# 词法分析器目标文件
LEXER_OBJS = $(BUILD_DIR)/token.o $(BUILD_DIR)/error.o $(BUILD_DIR)/lex.yy.o $(BUILD_DIR)/main.o

# 语法分析器目标文件（不含main.o，用parser_test.o代替）
PARSER_OBJS = $(BUILD_DIR)/token.o $(BUILD_DIR)/error.o $(BUILD_DIR)/lex.yy.o \
              $(BUILD_DIR)/ast.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/parser_test.o

# 完整编译器目标文件（pascc - Pascal-S to C Compiler）
PASCC_OBJS = $(BUILD_DIR)/token.o $(BUILD_DIR)/error.o $(BUILD_DIR)/lex.yy.o \
             $(BUILD_DIR)/ast.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/semantic.o \
             $(BUILD_DIR)/symbol.o $(BUILD_DIR)/codegen.o $(BUILD_DIR)/main.o

# 代码生成测试程序目标文件
CODEGEN_TEST_OBJS = $(BUILD_DIR)/token.o $(BUILD_DIR)/error.o $(BUILD_DIR)/lex.yy.o \
                    $(BUILD_DIR)/ast.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/semantic.o \
                    $(BUILD_DIR)/symbol.o $(BUILD_DIR)/codegen.o $(BUILD_DIR)/codegen_test.o

# 可执行文件
TARGET_LEXER = paslex
TARGET_PARSER = parser_test
TARGET_PASCC = pascc
TARGET_CODEGEN = codegen_test

# 默认目标：编译完整的Pascal-S到C翻译器
all: $(TARGET_PASCC)

# 词法分析器（测试用）
lexer: $(TARGET_LEXER)

# 新增目标：编译语法分析器测试程序
parser: $(TARGET_PARSER)

# 代码生成测试程序
codegen: $(TARGET_CODEGEN)

# 同时编译所有
both: $(TARGET_LEXER) $(TARGET_PARSER) $(TARGET_PASCC) $(TARGET_CODEGEN)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Flex编译
$(LEX_GEN): $(LEX_SRC) | $(BUILD_DIR)
	$(LEX) -o $@ $<

# 编译C文件（通用规则）
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译Flex生成的文件
$(BUILD_DIR)/lex.yy.o: $(LEX_GEN)
	$(CC) $(CFLAGS) -c -o $@ $<

# 链接词法分析器
$(TARGET_LEXER): $(LEXER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 链接语法分析器测试程序
$(TARGET_PARSER): $(PARSER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 链接完整的Pascal-S到C翻译器
$(TARGET_PASCC): $(PASCC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 链接代码生成测试程序
$(TARGET_CODEGEN): $(CODEGEN_TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 测试词法分析器（原有）
test-lexer: $(TARGET_LEXER)
	@echo "========== 测试词法分析器 =========="
	./$(TARGET_LEXER) $(TEST_DIR)/test1.pas -v
	@echo ""
	./$(TARGET_LEXER) $(TEST_DIR)/test2.pas -v
	@echo ""
	./$(TARGET_LEXER) $(TEST_DIR)/test_error.pas -v

# 测试语法分析器（新增）
test-parser: $(TARGET_PARSER)
	@echo "========== 测试语法分析器 =========="
	./$(TARGET_PARSER) $(TEST_DIR)/test1.pas
	@echo ""
	./$(TARGET_PARSER) $(TEST_DIR)/test2.pas
	@echo ""
	./$(TARGET_PARSER) $(TEST_DIR)/test_error.pas

# 测试所有（新增）
test-all: test-lexer test-parser test-codegen test-pascc

# 测试完整翻译器（新增）
test-pascc: $(TARGET_PASCC)
	@echo "========== 测试Pascal-S到C翻译器 =========="
	./$(TARGET_PASCC) -i $(TEST_DIR)/test1.pas -v
	@echo ""
	./$(TARGET_PASCC) -i $(TEST_DIR)/semantic/semantic_correct.pas -v

# 测试代码生成器（新增）
test-codegen: $(TARGET_CODEGEN)
	@echo "========== 测试代码生成器 =========="
	./$(TARGET_CODEGEN) $(TEST_DIR)/test1.pas
	@echo ""
	./$(TARGET_CODEGEN) $(TEST_DIR)/semantic/semantic_correct.pas

# 清理
clean:
	rm -rf $(BUILD_DIR) $(TARGET_LEXER) $(TARGET_PARSER) $(TARGET_PASCC) $(TARGET_CODEGEN) *.c

# 重新构建
rebuild: clean all

# 重新构建所有
rebuild-all: clean both

# 安装到系统路径（可选）
install: $(TARGET_PASCC)
	install -m 755 $(TARGET_PASCC) /usr/local/bin/

# 头歌平台：编译并输出到指定目录
educoder: $(TARGET_PASCC)
	mkdir -p /data/workspace/myshixun/bin
	install -m 755 $(TARGET_PASCC) /data/workspace/myshixun/bin/pascc

.PHONY: all lexer parser codegen both clean test-lexer test-parser test-codegen test-pascc test-all rebuild rebuild-all install
