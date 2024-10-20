#include "compiler.h"
int compile_file(const char *filename, const char *out_filename, int flags){
    struct compile_process* process=compile_process_create(filename, out_filename, flags);
    if(!process){
        return COMPILOR_FAILED_WITH_ERRORS;
    }
    //语法分析
    //词法分析
    //代码生成
    return COMPILOR_FILE_COMPLETE_OK;
} 