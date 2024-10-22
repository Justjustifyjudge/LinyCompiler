#ifndef LINYCOMPILOR_H
#define LINYCOMPILOR_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

//判断两个char*是否相等的宏
#define S_EQ(str1, str2) (str1&&str2&&(strcmp(str1, str2)==0))

//标志编译文件（文件名）的第几行第几列
struct pos{
    int line;
    int col;
    const char* filename;
};

#define NUMERIC_CASE \
    case '0': \
    case '1': \
    case '2': \
    case '3': \
    case '4': \
    case '5': \
    case '6': \
    case '7': \
    case '8': \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+': \
    case '-': \
    case '*': \
    case '>': \
    case '<': \
    case '^': \
    case '%': \
    case '!': \
    case '=': \
    case '~': \
    case '|': \
    case '&': \
    case '(': \
    case '[': \
    case ',': \
    case '.': \
/*
原代码：
    case '?'
修改原因：
    原代码中没有包含'/'，导致编译发生错误“未知的运算符”
    林一凡，2024.10.22，20点11分
*/  \
    case '?': \
    case '/'

#define SYMBOL_CASE \
    case '{': \
    case '}': \
    case ';': \
    case ':': \
    case '#': \
    case '\\': \
    case ')': \
    case ']'

enum{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_AYALYSIS_INPUT_ERROR
};

enum{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

struct token
{
    int type;
    int flags;
    struct pos pos;
    union{
        char cval;
        const char* sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };
    //与下一个token之间是否有空白需要跳过
    bool whitespace;

    const char* between_brackets;


};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process* process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process* process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process* process, char c);

struct lex_process_functions{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process{
    struct pos pos;
    struct vector* token_vec;
    struct compile_process* compiler;

    int current_expression_count;
    struct buffer* parentheses_buffer;
    struct lex_process_functions* functions;

    void* private;
    
};

enum{
    COMPILOR_FILE_COMPLETE_OK,
    COMPILOR_FAILED_WITH_ERRORS
};

struct compile_process
{
    // 标志文件应该如何被编译的标志位，比如-o, -c, -S等
    int flags;
    // 记录编译到的位置和编译文件名等信息
    struct pos pos;
    // fp即被打开编译的文件，abs_path是文件的绝对路径
    struct compile_process_input_file
    {
        FILE* fp;
        const char* abs_path;
    } cfile;
    // ofile是编译后的输出文件
    FILE* ofile;
    
};


int compile_file(const char *filename, const char *output_filename, int flags);
struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);

char compile_process_next_char(struct lex_process* lex_process);
char compile_process_peek_char(struct lex_process* lex_process);
void compile_process_push_char(struct lex_process* lex_process, char c);

void compiler_error(struct compile_process* compiler, const char* msg, ...);
void compiler_warning(struct compile_process* compiler, const char* msg, ...);

struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* functions, void* private);
void lex_process_free(struct lex_process* process);
void* lex_process_private(struct lex_process* process);
struct vector* lex_process_tokens(struct lex_process* process);

int lex(struct lex_process* process);

bool token_is_keyword(struct token *token, const char* value);

#endif // LINYCOMPILOR_H