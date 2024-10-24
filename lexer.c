#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

//通过exp条件判断是否继续读取字符到buffer的宏
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

struct token *read_next_token();

static struct lex_process *lex_process;
static struct token tmp_token;

static char peekc()
{
    return lex_process->functions->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->functions->next_char(lex_process);
    lex_process->pos.col += 1;
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }
    return c;
}

static char pushc(char c)
{
    lex_process->functions->push_char(lex_process, c);
}

static struct pos lex_file_position()
{
    return lex_process->pos;
}

struct token *token_create(struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}
struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

static struct token *token_make_string(char start_delim, char end_delim)
{
    struct buffer *buffer = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc(); // 读取双引号后第一个字符
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // 转义字符处理
            continue;
        }

        buffer_write(buffer, c);
    }

    buffer_write(buffer, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buffer)});
}

static bool op_treated_as_one(char c)
{
    return c == '(' || c == '[' || c == '.' || c == '*' || c == '?' || c == ',';
}
static bool is_single_operator(char c)
{
    return c == '+' || c == '-' || c == '/' || c == '*' ||
           c == '=' || c == '<' || c == '>' || c == '|' ||
           c == '&' || c == '^' || c == '%' || c == '!' ||
           c == '(' || c == '[' || c == ',' || c == '.' ||
           c == '~' || c == '?';
}
//判断是否是合法的运算符
bool op_valid(const char *op){
    return  S_EQ(op, "+")||
            S_EQ(op, "-")||
            S_EQ(op, "*")||
            S_EQ(op, "/")||
            S_EQ(op, "!")||
            S_EQ(op, "^")||
            S_EQ(op, "+=")||
            S_EQ(op, "-=")||
            S_EQ(op, "*=")||
            S_EQ(op, "/=")||
            S_EQ(op, ">>")||
            S_EQ(op, "<<")||
            S_EQ(op, ">=")||
            S_EQ(op, "<=")||
            S_EQ(op, ">")||
            S_EQ(op, "<")||
            S_EQ(op, "||")||
            S_EQ(op, "&&")||
            S_EQ(op, "|")||
            S_EQ(op, "&")||
            S_EQ(op, "++")||
            S_EQ(op, "--")||
            S_EQ(op, "=")||
            S_EQ(op, "!=")||
            S_EQ(op, "==")||
            S_EQ(op, "->")||
            S_EQ(op, "(")||
            S_EQ(op, "[")||
            S_EQ(op, ",")||
            S_EQ(op, ".")||
            S_EQ(op, "...")||
            S_EQ(op, "~")||
            S_EQ(op, "?")||
            S_EQ(op, "%");
}
void read_op_flush_back_keep_first(struct buffer* buffer){
    const char* data=buffer_ptr(buffer);
    int len=buffer->len;
    for(int i=len-1;i>=1;i--){
        if(data[i]==0x00){
            continue;
        }
        pushc(data[i]);
    }
}
const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);
    //如果不是单目运算符，则通过peekc()读取下一个字符
    if (!op_treated_as_one(op))
    {
        op = peekc();
        if(is_single_operator(op)){
            buffer_write(buffer, op);
            nextc();
            single_operator = false;
        }
    }

    buffer_write(buffer, 0x00);
    char* ptr=buffer_ptr(buffer);
    if(!single_operator){
        if(!op_valid(ptr)){
            read_op_flush_back_keep_first(buffer);
            ptr[1]=0x00;
        }
    } else if(!op_valid(ptr)){
        compiler_error(lex_process->compiler, "未知的运算符：%s\n",ptr);
    }
    return ptr;
}

static void lex_new_expression(){
    lex_process->current_expression_count++;
    if(lex_process->current_expression_count==1){
        lex_process->parentheses_buffer=buffer_create();
    }
}

static void lex_finish_expression(){
    lex_process->current_expression_count--;
    if(lex_process->current_expression_count<0){
        compiler_error(lex_process->compiler, "没有找到匹配的开始括号\n");
    }
}

bool lex_is_in_expression(){
    return lex_process->current_expression_count>0;
}
bool is_keyword(char* str){
    return S_EQ(str, "unsigned")||
           S_EQ(str, "signed")||
           S_EQ(str, "char")||
           S_EQ(str, "int")||
           S_EQ(str, "short")||
           S_EQ(str, "long")||
           S_EQ(str, "float")||
           S_EQ(str, "double")||
           S_EQ(str, "void")||
           S_EQ(str, "struct")||
           S_EQ(str, "union")||
           S_EQ(str, "static")||
           S_EQ(str, "__ignore_typecheck")||
           S_EQ(str, "return")||
           S_EQ(str, "include")||
           S_EQ(str, "sizeof")||
           S_EQ(str, "if")||
           S_EQ(str, "else")||
           S_EQ(str, "while")||
           S_EQ(str, "for")||
           S_EQ(str, "do")||
           S_EQ(str, "break")||
           S_EQ(str, "continue")||
           S_EQ(str, "switch")||
           S_EQ(str, "case")||
           S_EQ(str, "default")||
           S_EQ(str, "goto")||
           S_EQ(str, "typedef")||
           S_EQ(str, "const")||
           S_EQ(str, "extern")||
           S_EQ(str, "restrict");
}

static struct token *token_make_operator_or_string()
{
    char op=peekc();
    if(op=='<'){
        struct token* last_token=lexer_last_token();
        //处理形如 #include<lyf.h> 的情况
        if(token_is_keyword(last_token,"include")){
            return token_make_string('<','>');
        }
    }
    struct token *token = token_create(&(struct token){.type=TOKEN_TYPE_OPERATOR,.sval=read_op()});
    if(op=='('){
        lex_new_expression();
    }

    return token;
}
static struct token *token_make_symbol(){
    char c=nextc();
    if(c==')'){
        lex_finish_expression();
    }
    struct token *token = token_create(&(struct token){.type=TOKEN_TYPE_SYMBOL,.cval=c});
    return token;
}

//读取单行注释完成词法token
struct token* token_make_one_line_comment()
{
    struct buffer* buffer=buffer_create();
    char c=0;
    LEX_GETC_IF(buffer,c,c!='\n'&&c!='\r'&&c!=EOF);
    return token_create(&(struct token){.type=TOKEN_TYPE_COMMENT,.sval=buffer_ptr(buffer)});
};
//读取多行注释完成词法token
struct token* token_make_multiline_comment(){
    struct buffer* buffer=buffer_create();
    char c=0;
    while(1){
        LEX_GETC_IF(buffer,c,c!='*'&&c!=EOF);
        //读到代码文件结尾还没有结束注释则报错
        if(c==EOF){
            compiler_error(lex_process->compiler,"注释没有匹配的结束符\n");
        }
        //读到了"*"号
        else if (c=='*')
        {
            //跳过*号
            nextc();
            //如果下一个字符是"/"则结束注释
            if(peekc()=='/'){
                nextc();
                break;
            }
        }
    }
    return token_create(&(struct token){.type=TOKEN_TYPE_COMMENT,.sval=buffer_ptr(buffer)});
}
struct token* handle_comment(){
    char c=peekc();
    if(c=='/'){
        nextc();
        if(peekc()=='/'){
            nextc();
            return token_make_one_line_comment();
        }
        else if(peekc()=='*'){
            nextc();
            return token_make_multiline_comment();
        }

        pushc('/');
        return token_make_operator_or_string();
    }
    return NULL;
}

static struct token* token_make_identifier_or_keyword(){
    struct buffer* buffer=buffer_create();
    char c=0;
    //读取变量名或关键字内容
    LEX_GETC_IF(buffer, c, (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_');
    
    buffer_write(buffer,0x00);

    //检查是否是关键字
    if(is_keyword(buffer_ptr(buffer))){
        return token_create(&(struct token){.type=TOKEN_TYPE_KEYWORD,.sval=buffer_ptr(buffer)});
    }
    return token_create(&(struct token){.type=TOKEN_TYPE_IDENTIFIER,.sval=buffer_ptr(buffer)});
}

struct token* read_special_token(){
    
    char c=peekc();
    //遇到字母或者下划线打头的不是标识符（变量名）就是关键字
    if(isalpha(c)||c=='_'){
        return token_make_identifier_or_keyword();
    }
}
struct token* token_make_newline(){
    nextc();
    return token_create(&(struct token){.type=TOKEN_TYPE_NEWLINE});
}
struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    //先尝试一下看看是不是无效的代码（注释）
    token = handle_comment();
    if (token != NULL){
        return token;
    }
    //如果不是注释再根据字符类型来生成token
    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        break;
    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;
    SYMBOL_CASE:
        token= token_make_symbol();
        break;
    case '"':
        token = token_make_string('"', '"');
        break;
    case ' ':
    case '\t':
        token = handle_whitespace();
        break;
    case '\n':
    //由于我是Windows系统，所以在编译遇到\r换行符时也要进行处理
    //原代码中没有处理\r换行符，
    //    即没有
    //case '\r':
    //    这一行
    case '\r':
        token=token_make_newline();
        break;
    case EOF:
        // 结束代码文本的全部词法分析
        break;

    default:
        token=read_special_token();
        if(!token){
            compiler_error(lex_process->compiler, "未知的字符\n");
        }
    }
    return token;
};

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }
    return LEXICAL_ANALYSIS_ALL_OK;
}