#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void){
    char buffer[1000];
    printf("enter a command:\n");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("your command is: \n%s\n",buffer);
    char *const argv[]={
        buffer,
        NULL
    };
    pid_t pid=fork(); 
    if(pid>0){
        printf( "father process id:,%ld\n", (long)getpid());
        printf( "son process id: ,%ld\n", (long)pid);
        waitpid(pid,NULL,0);
    }
    else if(pid==0){
        printf( "son process id:,%ld\n", (long)getpid());
        execvp(buffer,argv);
        perror("execvp");
        
    }
    else{
        printf("an error accrued!\n");
    }
    return 0;
}