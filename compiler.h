#ifndef LINYCOMPILOR_H
#define LINYCOMPILOR_H

#include <stdio.h>
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