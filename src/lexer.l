/**
 * @file lexer.l
 * @brief Pascal-S 语言词法分析器 Flex 规则文件
 * @author 组员1：词法分析器开发
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "token.h"
#include "error.h"
#include "lexer.h"

TokenValue yylval;
int yycolumn = 1;

static const char *current_filename = NULL;
static FILE *current_file = NULL;
static int token_count = 0;

#define UPDATE_COLUMN() (yycolumn += yyleng)
#define RESET_COLUMN() (yycolumn = 1)

static int get_token_start_column(void) {
    return yycolumn - yyleng;
}

%}

%option noyywrap
%option yylineno
%option nodefault

LETTER          [a-zA-Z_]
DIGIT           [0-9]
IDENTIFIER      {LETTER}({LETTER}|{DIGIT})*
INTEGER         {DIGIT}+
REAL            {DIGIT}+\.{DIGIT}+
CHAR_CONST      '[^']'
COMMENT         \{[^}]*\}
UNTERMINATED_COMMENT \{[^}]*
LINE_COMMENT    \/\/[^\n]*

WHITESPACE      [ \t\r]+
NEWLINE         \n

ASSIGN          \:\=
NE              \<\>
LE              \<\=
GE              \>\=
DOUBLEDOT       \.\.

PLUS            \+
MINUS           \-
MULTIPLY        \*
DIVIDE          \/
EQ              \=
LT              \<
GT              \>
SEMICOLON       \;
COMMA           \,
DOT             \.
LPAREN          \(
RPAREN          \)
LBRACKET        \[
RBRACKET        \]
COLON           \:

ILLEGAL_CHAR    .

%%

[Pp][Rr][Oo][Gg][Rr][Aa][Mm]       { UPDATE_COLUMN(); return TOKEN_PROGRAM; }
[Vv][Aa][Rr]                       { UPDATE_COLUMN(); return TOKEN_VAR; }
[Cc][Oo][Nn][Ss][Tt]               { UPDATE_COLUMN(); return TOKEN_CONST; }
[Pp][Rr][Oo][Cc][Ee][Dd][Uu][Rr][Ee] { UPDATE_COLUMN(); return TOKEN_PROCEDURE; }
[Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]   { UPDATE_COLUMN(); return TOKEN_FUNCTION; }
[Bb][Ee][Gg][Ii][Nn]               { UPDATE_COLUMN(); return TOKEN_BEGIN; }
[Ee][Nn][Dd]                       { UPDATE_COLUMN(); return TOKEN_END; }
[Ii][Ff]                           { UPDATE_COLUMN(); return TOKEN_IF; }
[Tt][Hh][Ee][Nn]                   { UPDATE_COLUMN(); return TOKEN_THEN; }
[Ee][Ll][Ss][Ee]                   { UPDATE_COLUMN(); return TOKEN_ELSE; }
[Ff][Oo][Rr]                       { UPDATE_COLUMN(); return TOKEN_FOR; }
[Tt][Oo]                           { UPDATE_COLUMN(); return TOKEN_TO; }
[Dd][Oo]                           { UPDATE_COLUMN(); return TOKEN_DO; }
[Rr][Ee][Aa][Dd]                   { UPDATE_COLUMN(); return TOKEN_READ; }
[Ww][Rr][Ii][Tt][Ee][Ll][Nn]       { UPDATE_COLUMN(); return TOKEN_WRITELN; }
[Ww][Rr][Ii][Tt][Ee]               { UPDATE_COLUMN(); return TOKEN_WRITE; }
[Aa][Rr][Rr][Aa][Yy]               { UPDATE_COLUMN(); return TOKEN_ARRAY; }
[Oo][Ff]                           { UPDATE_COLUMN(); return TOKEN_OF; }
[Ii][Nn][Tt][Ee][Gg][Ee][Rr]       { UPDATE_COLUMN(); return TOKEN_INTEGER; }
[Rr][Ee][Aa][Ll]                   { UPDATE_COLUMN(); return TOKEN_REAL; }
[Bb][Oo][Oo][Ll][Ee][Aa][Nn]       { UPDATE_COLUMN(); return TOKEN_BOOLEAN; }
[Cc][Hh][Aa][Rr]                   { UPDATE_COLUMN(); return TOKEN_CHAR; }
[Dd][Ii][Vv]                       { UPDATE_COLUMN(); return TOKEN_DIV; }
[Mm][Oo][Dd]                       { UPDATE_COLUMN(); return TOKEN_MOD; }
[Aa][Nn][Dd]                       { UPDATE_COLUMN(); return TOKEN_AND; }
[Oo][Rr]                           { UPDATE_COLUMN(); return TOKEN_OR; }
[Nn][Oo][Tt]                       { UPDATE_COLUMN(); return TOKEN_NOT; }
[Ww][Hh][Ii][Ll][Ee]               { UPDATE_COLUMN(); return TOKEN_WHILE; }
[Tt][Rr][Uu][Ee]                   { UPDATE_COLUMN(); 
                                      strncpy(yylval.str_val, yytext, sizeof(yylval.str_val) - 1);
                                      return TOKEN_TRUE; }
[Ff][Aa][Ll][Ss][Ee]               { UPDATE_COLUMN(); 
                                      strncpy(yylval.str_val, yytext, sizeof(yylval.str_val) - 1);
                                      return TOKEN_FALSE; }

{IDENTIFIER}    {
                    UPDATE_COLUMN();
                    if (yyleng > 255) {
                        add_error(LEX_ERROR_STRING_TOO_LONG, yylineno, 
                                  get_token_start_column(),
                                  "Identifier too long: %s", yytext);
                    }
                    strncpy(yylval.str_val, yytext, sizeof(yylval.str_val) - 1);
                    yylval.str_val[sizeof(yylval.str_val) - 1] = '\0';
                    return TOKEN_IDENTIFIER;
                }

{INTEGER}       {
                    UPDATE_COLUMN();
                    yylval.int_val = atoi(yytext);
                    return TOKEN_INTEGER_CONST;
                }

{REAL}          {
                    UPDATE_COLUMN();
                    yylval.real_val = atof(yytext);
                    return TOKEN_REAL_CONST;
                }

{CHAR_CONST}    {
                    UPDATE_COLUMN();
                    yylval.char_val = yytext[1];
                    return TOKEN_CHAR_CONST;
                }

{ASSIGN}        { UPDATE_COLUMN(); return TOKEN_ASSIGN; }
{NE}            { UPDATE_COLUMN(); return TOKEN_NE; }
{LE}            { UPDATE_COLUMN(); return TOKEN_LE; }
{GE}            { UPDATE_COLUMN(); return TOKEN_GE; }
{DOUBLEDOT}     { UPDATE_COLUMN(); return TOKEN_DOUBLEDOT; }

{PLUS}          { UPDATE_COLUMN(); return TOKEN_PLUS; }
{MINUS}         { UPDATE_COLUMN(); return TOKEN_MINUS; }
{MULTIPLY}      { UPDATE_COLUMN(); return TOKEN_MULTIPLY; }
{DIVIDE}        { UPDATE_COLUMN(); return TOKEN_DIVIDE; }
{EQ}            { UPDATE_COLUMN(); return TOKEN_EQ; }
{LT}            { UPDATE_COLUMN(); return TOKEN_LT; }
{GT}            { UPDATE_COLUMN(); return TOKEN_GT; }

{SEMICOLON}     { UPDATE_COLUMN(); return TOKEN_SEMICOLON; }
{COMMA}         { UPDATE_COLUMN(); return TOKEN_COMMA; }
{DOT}           { UPDATE_COLUMN(); return TOKEN_DOT; }
{LPAREN}        { UPDATE_COLUMN(); return TOKEN_LPAREN; }
{RPAREN}        { UPDATE_COLUMN(); return TOKEN_RPAREN; }
{LBRACKET}      { UPDATE_COLUMN(); return TOKEN_LBRACKET; }
{RBRACKET}      { UPDATE_COLUMN(); return TOKEN_RBRACKET; }
{COLON}         { UPDATE_COLUMN(); return TOKEN_COLON; }

{LINE_COMMENT}  { UPDATE_COLUMN(); /* skip // single-line comment */ }

"(*"            {
                    /* (* ... *) style comment */
                    int c;
                    UPDATE_COLUMN();
                    int comment_start_line = yylineno;
                    while (1) {
                        c = input();
                        if (c == EOF || c == 0) {
                            add_error(LEX_ERROR_UNTERMINATED_COMMENT, comment_start_line,
                                      get_token_start_column(),
                                      "Unterminated comment: missing '*)');");
                            break;
                        }
                        if (c == '\n') {
                            yylineno++;
                            RESET_COLUMN();
                        } else {
                            yycolumn++;
                        }
                        if (c == '*') {
                            int next = input();
                            if (next == ')') {
                                yycolumn++;
                                break;
                            } else if (next == '\n') {
                                yylineno++;
                                RESET_COLUMN();
                            } else if (next == EOF || next == 0) {
                                add_error(LEX_ERROR_UNTERMINATED_COMMENT, comment_start_line,
                                          get_token_start_column(),
                                          "Unterminated comment: missing '*)');");
                                break;
                            } else {
                                yycolumn++;
                            }
                        }
                    }
                }

{COMMENT}       {
                    int i;
                    for (i = 0; i < yyleng; i++) {
                        if (yytext[i] == '\n') {
                            yylineno++;
                            RESET_COLUMN();
                        }
                    }
                    UPDATE_COLUMN();
                }

{UNTERMINATED_COMMENT} {
                    add_error(LEX_ERROR_UNTERMINATED_COMMENT, yylineno,
                              get_token_start_column(),
                              "Unterminated comment: missing '}'");
                    UPDATE_COLUMN();
                }

{WHITESPACE}    { UPDATE_COLUMN(); }
{NEWLINE}       { yylineno++; RESET_COLUMN(); }

{ILLEGAL_CHAR}  {
                    UPDATE_COLUMN();
                    add_error(LEX_ERROR_ILLEGAL_CHAR, yylineno,
                              get_token_start_column(),
                              "Illegal character: '%s' (ASCII: %d)", 
                              yytext, (unsigned char)yytext[0]);
                    return TOKEN_ERROR;
                }

%%

int init_lexer(const char *filename) {
    current_filename = filename;
    current_file = fopen(filename, "r");
    
    if (current_file == NULL) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }
    
    yyin = current_file;
    yylineno = 1;
    yycolumn = 1;
    token_count = 0;
    
    init_error_list();
    
    return 0;
}

void close_lexer(void) {
    if (current_file != NULL) {
        fclose(current_file);
        current_file = NULL;
    }
    
    free_error_list();
}

TokenType get_next_token(Token *token) {
    if (token == NULL) {
        return TOKEN_ERROR;
    }
    
    int lex_result = yylex();
    TokenType type;
    
    /* flex returns 0 at EOF, map it to TOKEN_EOF */
    if (lex_result == 0) {
        type = TOKEN_EOF;
    } else {
        type = (TokenType)lex_result;
    }
    
    token->type = type;
    token->line = yylineno;
    token->column = get_token_start_column();
    
    memcpy(&token->value, &yylval, sizeof(TokenValue));
    
    token_count++;
    
    return type;
}

void print_token(const Token *token) {
    if (token == NULL) {
        printf("Token: NULL\n");
        return;
    }
    
    printf("Token #%d: ", token_count);
    printf("[Line:%d, Col:%d] ", token->line, token->column);
    printf("%-15s ", token_type_to_string(token->type));
    
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            printf("Value: %s", token->value.str_val);
            break;
        case TOKEN_INTEGER_CONST:
            printf("Value: %d", token->value.int_val);
            break;
        case TOKEN_REAL_CONST:
            printf("Value: %f", token->value.real_val);
            break;
        case TOKEN_CHAR_CONST:
            printf("Value: '%c'", token->value.char_val);
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            printf("Value: %s", token->value.str_val);
            break;
        default:
            break;
    }
    
    printf("\n");
}

void test_lexer(void) {
    Token token;
    TokenType type;
    
    printf("========== Lexer Analysis Start ==========\n");
    printf("File: %s\n\n", current_filename);
    
    while ((type = get_next_token(&token)) != TOKEN_EOF) {
        print_token(&token);
        
        if (type == TOKEN_ERROR) {
            printf("  [Error Recovery: Continuing scan]\n");
        }
    }
    
    printf("\n========== Lexer Analysis End ==========\n");
    printf("Total tokens: %d\n", token_count);
    
    if (has_errors()) {
        print_errors(stdout);
    }
}

const char* get_token_display_name(TokenType type) {
    return token_type_to_string(type);
}
