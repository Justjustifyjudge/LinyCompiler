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
bool lex_is_in_expression();

static struct lex_process *lex_process;
static struct token tmp_token;

static char peekc()
{
    return lex_process->functions->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->functions->next_char(lex_process);
    
    if(lex_is_in_expression()){
        buffer_write(lex_process->parentheses_buffer, c);
    }
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

//如果没有匹配到对应的下一个字符，发生assert
static char assert_next_char(char c){
    char next = nextc();
    assert(next == c);
    return next;
}
static struct pos lex_file_position()
{
    return lex_process->pos;
}

struct token *token_create(struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    if(lex_is_in_expression()){
        tmp_token.between_brackets=buffer_ptr(lex_process->parentheses_buffer);
    }
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

int lexer_number_type(char c){
    int res=NUMBER_TYPE_NORMAL;
    if(c=='L'||c=='l'){
        res = NUMBER_TYPE_LONG;
    } else if(c=='F'||c=='f'){
        res = NUMBER_TYPE_FLOAT;
    }
    return res;
}
struct token *token_make_number_for_value(unsigned long number)
{
    int number_type=lexer_number_type(peekc());
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
char lex_get_escape_char(char c){
    char co=0;
    switch(c){
        case 'n':co='\n';break;
        case 'r':co='\r';break;
        case 't':co='\t';break;
        case '\\':co='\\';break;
        case '\'':co='\'';break;
    }
    return co;
}
void lexer_pop_token(){
    vector_pop(lex_process->token_vec);
}
//判断哪些字符是16进制数中合法的字符
bool is_hex_char(char c){
    return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}
//读取十六进制数的整条字符串
const char* read_hex_number_str(){
    struct buffer* buffer=buffer_create();
    char c=peekc();
    LEX_GETC_IF(buffer,c,is_hex_char(c));
    //写入终止符
    buffer_write(buffer,0x00);
    return buffer_ptr(buffer);
}
//处理十六进制类型的token
struct token* token_make_special_number_hexadecimal(){
    //跳过'x'||'X'
    nextc();
    unsigned long number=0;
    const char* number_str=read_hex_number_str();
    number=strtoul(number_str,0,16);
    return token_make_number_for_value(number);
}

void lexer_validate_binary_string(const char* str){
    size_t len=strlen(str);
    for(int i=0;i<len;i++){
        if(str[i]!='0'&&str[i]!='1'){
            compiler_error(lex_process->compiler,"非法的字符'%c',二进制数只能由0和1组成");
        }
    }
}
struct token* token_make_number_binary(){
    //跳过'b'
    nextc();
    unsigned long number=0;
    const char* number_str=read_number_str();
    lexer_validate_binary_string(number_str);
    number=strtoul(number_str,0,2);
    return token_make_number_for_value(number);
}
//处理十六进制、八进制等特殊进制的数字
struct token* token_make_special_number(){
    struct token* token=NULL;
    struct token* last_token=lexer_last_token();
    //如果没有前导零，说明是以'x'、'b'、'h'开头的变量名
    if(!last_token||!(last_token->type==TOKEN_TYPE_NUMBER&&last_token->llnum==0)){
        return token_make_identifier_or_keyword();
    }
    //把识别符0x、0b、0h最前面的0去掉
    lexer_pop_token();
    
    char c=peekc();
    if(c=='x'||c=='X'){
        token=token_make_special_number_hexadecimal();
    } else if(c=='b'||c=='B'){
        token=token_make_number_binary();
    }
    
    return token;
}
//处理单引号内包括的字符
struct token* token_make_quote(){
    assert_next_char('\'');//确保下一个字符是单引号
    char c=nextc();
    //转义字符
    if(c=='\\'){
        c=nextc();
        c=lex_get_escape_char(c);
    }
    //如果单引号内超过了一个字符还没结束单引号
    if(nextc()!='\''){
        compiler_error(lex_process->compiler,"没有匹配结束的单引号或者单引号内超过了一个字符或者单引号内没有字符\n");
    }
    //返回的是0~255的字符
    return token_create(&(struct token){.type=TOKEN_TYPE_NUMBER,.cval=c});
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
    case 'b':
    case 'B':
    case 'x':
    case 'X':
        token = token_make_special_number();
        break;
    case '"':
        token = token_make_string('"', '"');
        break;
    case '\'':
        token = token_make_quote();
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

char lexer_string_buffer_next_char(struct lex_process* process){
    struct buffer* buffer=lex_process_private(process);
    return buffer_read(buffer);
}
char lexer_string_buffer_peek_char(struct lex_process* process){
    struct buffer* buffer=lex_process_private(process);
    return buffer_peek(buffer);
}
void lexer_string_buffer_push_char(struct lex_process* process, char c){
    struct buffer* buffer=lex_process_private(process);
    buffer_write(buffer, c);
}
struct lex_process_functions lexer_string_buffer_functions={
    .next_char=lexer_string_buffer_next_char,
    .peek_char=lexer_string_buffer_peek_char,
    .push_char=lexer_string_buffer_push_char
};
struct lex_process* tokens_build_for_string(struct compile_process* compiler, const char* str){
    struct buffer* buffer=buffer_create();
    buffer_printf(buffer, str);
    struct lex_process* lex_process=lex_process_create(compiler, &lexer_string_buffer_functions, buffer);
    if(!lex_process){
        return NULL;
    }
    if(lex(lex_process)!=LEXICAL_ANALYSIS_ALL_OK){
        return NULL;
    }
    return lex_process;
}