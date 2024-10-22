#include <stdio.h>
#include "helpers/vector.h"
#include "compiler.h"
int main(){
    //编译程序
    int res=compile_file("./test.c", "./test",0);
    //获取编译返回信息
    if(res==COMPILOR_FILE_COMPLETE_OK){
        printf("编译完成\n");
    } else if(res==COMPILOR_FAILED_WITH_ERRORS){
        printf("发生了已知错误\n");
    } else {
        printf("发生了未知的错误\n");
    }
    
    return 0;
}