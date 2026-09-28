#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#define MAX_BUFFER 10000
#define MAX_ARGS 64

int main(void){
    //variables
    char buffer[MAX_BUFFER];
    char path[MAX_BUFFER];
    char *argv[MAX_ARGS];
    
    while(1){
        int arg_count = 0;
        int syntax_error = 0;
        char *p = buffer;
        char *output_file = NULL;
        if (getcwd(path, sizeof(path)) == NULL) {
            perror("getcwd");
        }
        printf("shall:%s %% ", path);
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
        argv[arg_count] = NULL;
        if(arg_count==0)
            continue;
        for(int i=0; i<arg_count;i++){
            if(strcmp(argv[i], ">") == 0){
                if (i == 0) {
                    fprintf(stderr, "shall: expected command before >\n");
                    syntax_error = 1;
                    break;
                }
                if(i + 1 < arg_count){
                    output_file=argv[i+1];
                    argv[i]=NULL;
                    break;
                }else{
                    fprintf(stderr, "shall: expected filename after >\n");
                    syntax_error = 1;
                    break;
                }
            }
        }
        if (syntax_error)
            continue;
        //checks for exit
        if (strcmp(argv[0], "exit")==0)//means they are equal 
            break;
        //checks for cd    
        if (strcmp(argv[0], "cd") == 0) {
            if (arg_count < 2) {
                printf("cd: missing argument\n");
                continue;
            }

            if (chdir(argv[1]) == -1) {
                perror("cd");
            }

            continue;
        }
        
        //pid part
        pid_t pid=fork(); 
        if(pid>0){
            waitpid(pid,NULL,0);
        }
        else if(pid==0){
            if(output_file != NULL){
                int fd = open(output_file,O_WRONLY | O_CREAT | O_TRUNC,0644);
                if(fd==-1){
                    perror(output_file);
                    _exit(1);
                }
                int fd2 = dup2(fd, STDOUT_FILENO);
                if(fd2==-1){
                       perror("dup2");
                    close(fd);
                    _exit(1);
                }
                close(fd);
            }
            execvp(argv[0],argv);
            if (errno == ENOENT) {
                fprintf(stderr, "shall: command not found: %s\n", argv[0]);
            } else {
                perror(argv[0]);
            }
            _exit(127);

        }
        else{
            perror("fork");
        } 
    }
    return 0;
}