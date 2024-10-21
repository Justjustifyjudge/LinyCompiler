#include "compiler.h"
#include "helpers/vector.h"
struct lex_process* lex_process;

static char peekc(){
    return lex_process->functions->peek_char(lex_process);
}

static char pushc(char c){
    lex_process->functions->push_char(lex_process,c);
}

struct token* read_next_token(){
    struct token* token=NULL;
    char c=peekc();
    switch (c)
    {
    case EOF:
        //结束代码文本的全部词法分析
        break;
    
    default:
        compiler_error(lex_process->compiler,"未知的字符\n");
        break;
    }
    return token;
};


int lex(struct lex_process* process){
    process->current_expression_count=0;
    process->parentheses_buffer=NULL;
    lex_process=process;
    process->pos.filename=process->compiler->cfile.abs_path;

    struct token* token=read_next_token();
    while(token){
        vector_push(process->token_vec, token);
        token=read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}