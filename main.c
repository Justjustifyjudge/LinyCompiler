#include <stdio.h>
#include "helpers/vector.h"
#include "compiler.h"
int main(){
    int res=compile_file("./test.c", "./test",0);
    if(res==COMPILOR_FILE_COMPLETE_OK){
        printf("everything is ok\n");
    } else if(res==COMPILOR_FAILED_WITH_ERRORS){
        printf("there are errors\n");
    } else {
        printf("Unknown error\n");
    }
    
    return 0;
}