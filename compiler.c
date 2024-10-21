#include "compiler.h"
#include <stdarg.h>

struct lex_process_functions compiler_lex_functions={
    .next_char=compile_process_next_char,
    .peek_char=compile_process_peek_char,
    .push_char=compile_process_push_char
};

void compiler_error(struct compile_process* compiler, const char* msg, ...){

}
int compile_file(const char *filename, const char *out_filename, int flags){
    struct compile_process* process=compile_process_create(filename, out_filename, flags);
    if(!process){
        return COMPILOR_FAILED_WITH_ERRORS;
    }
    //词法分析
    struct lex_process* lex_process=lex_process_create(process, &compiler_lex_functions, NULL);
    if(!lex_process){
        return COMPILOR_FAILED_WITH_ERRORS;
    }

    if(lex(lex_process)!=LEXICAL_ANALYSIS_ALL_OK){
        return COMPILOR_FAILED_WITH_ERRORS;
    }
    //语义分析
    //代码生成
    return COMPILOR_FILE_COMPLETE_OK;
} 