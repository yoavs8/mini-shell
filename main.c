#include "executor.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define MAX_BUFFER 10000

int main(void) {
    char buffer[MAX_BUFFER];
    char path[MAX_BUFFER];
    // infinite event loop so the shell remains active and can accept an unlimited sequence of
    // commands.
    while (1) {
        if (getcwd(path, sizeof(path)) == NULL) {
            perror("getcwd");
        }
        printf("shall:%s %% ", path);
        fflush(stdout);

        // gets command from user
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return 0;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        Command cmd = {0};
        int parse_result = parse_command(buffer, &cmd);
        if (parse_result <= 0)
            continue;
        if (strcmp(cmd.argv[0], "exit") == 0)
            break;

        if (strcmp(cmd.argv[0], "cd") == 0) {
            if (cmd.argv[1] == NULL) {
                printf("cd: missing argument\n");
                continue;
            }

            if (chdir(cmd.argv[1]) == -1) {
                perror("cd");
            }
            continue;
        }
        execute_command(&cmd);
    }
    return 0;
}