#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void){
  

    char *const argv[] = {
        "ls",
        "-l",
        NULL
    };

    printf("before fork\n");
    pid_t pid=fork(); 
    printf("after fork\n");
    if(pid>0){
        printf( "father process id:,%ld\n", (long)getpid());
        printf( "son process id: ,%ld\n", (long)pid);
        waitpid(pid,NULL,0);
    }
    else if(pid==0){
        sleep(2);
        printf( "son process id:,%ld\n", (long)getpid());
        execvp("ls",argv);
        
    }
    else{
        printf("an error accrued!\n");
    }
}