#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void){
    printf("before fork\n");
    pid_t pid=fork(); 
    printf("after fork\n");
    if(pid>0){
        printf( "father process id:,%ld\n", (long)getpid());
        printf( "son process id: ,%ld\n", (long)pid);
        waitpid(pid,&status,0);
    }
    else if(pid==0){
        printf( "son process id:,%ld\n", (long)getpid());
    }
    else{
        printf("an error accrued!\n");
    }
}