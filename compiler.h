#ifndef LINYCOMPILOR_H
#define LINYCOMPILOR_H

#include <stdio.h>
#include <stdbool.h>

struct pos{
    int line;
    int col;
    const char* filename;
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

    union{
        char cval;
        const char* sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void* any;
    };
    //
    bool whitespace;

    const char* between_brackets;


};


enum{
    COMPILOR_FILE_COMPLETE_OK,
    COMPILOR_FAILED_WITH_ERRORS
};

struct compile_process
{
    // 标志文件应该如何被编译的标志位
    int flags;

    struct compile_process_input_file
    {
        FILE* fp;
        const char* abs_path;
    } cfile;

    FILE* ofile;
    
};


int compile_file(const char *filename, const char *output_filename, int flags);
struct compile_process* compile_process_create(const char* filename, const char* filename_out, int flags);
#endif // LINYCOMPILOR_H