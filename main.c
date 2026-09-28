#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_BUFFER 1000
#define MAX_ARGS 64

int main(void){
    //variables
    char buffer[MAX_BUFFER];
    char *argv[MAX_ARGS];
    while(1){
        int arg_count = 0;
        char *p = buffer;
        printf("shall>");//name of shell
        fflush(stdout);

        //gets command from user
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return 0;
        }
        buffer[strcspn(buffer, "\n")] = '\0';

        //argv based on buffer: O(n)
        while (*p != '\0') {

            while (*p == ' ' || *p == '\t') {
                p++;
            }

            if (*p == '\0') 
                break;
            
            if(arg_count<MAX_ARGS - 1)
                argv[arg_count++] = p;

            while (*p != '\0' && *p != ' ' && *p != '\t') {
                p++;
            }

            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }

        if(arg_count==0)
            continue;

        //checks for exit
        if (!strcmp(argv[0], "exit"))//means they are equal 
            break;

        argv[arg_count] = NULL;
        //pid part
        pid_t pid=fork(); 
        if(pid>0){
            waitpid(pid,NULL,0);
        }
        else if(pid==0){
            execvp(argv[0],argv);
            perror("execvp");
            _exit(127);
        }
        else{
            perror("fork");
        } 
    }
    return 0;
}