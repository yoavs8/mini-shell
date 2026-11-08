#include "executor.h"
#include "parser.h"
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define MAX_BUFFER 10000
#define HOST_COLOR "\033[36m"  // cyan
#define PATH_COLOR "\033[34m"  // blue
#define SHELL_COLOR "\033[35m" // magenta
#define RESET_COLOR "\033[0m"  // white

int main(void) {
    char buffer[MAX_BUFFER];
    char path[MAX_BUFFER];
    char host_name[MAX_BUFFER];
    // infinite event loop so the shell remains active and can accept an unlimited sequence of
    // commands.
    while (1) {
        struct passwd *user;
        if (getcwd(path, sizeof(path)) == NULL) {
            perror("getcwd");
            printf("shall:");
        } else if (gethostname(host_name, sizeof(host_name)) == -1) {
            perror("gethostname");
            printf("shall:");
        } else if ((user = getpwuid(getuid())) == NULL) {
            fprintf(stderr, "shall: could not get username\n");
            printf("shall: %% ");
        } else {
            size_t len = strlen(user->pw_dir);
            if (strncmp(path, user->pw_dir, len) == 0 && (path[len] == '/' || path[len] == '\0')) {
                printf(SHELL_COLOR "shall: " HOST_COLOR "%s@%s:" PATH_COLOR "~%s" RESET_COLOR
                                   " %% ",
                       user->pw_name, host_name, path + len);
            } else {
                printf(SHELL_COLOR "shall: " HOST_COLOR "%s@%s:" PATH_COLOR "%s" RESET_COLOR " %% ",
                       user->pw_name, host_name, path);
            }
        }
        fflush(stdout);

        // gets command from user
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            return 0;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        pipeline pipe = {0};
        int parse_result = parse_pipeline(buffer, &pipe);
        if (parse_result <= 0)
            continue;
        if (strcmp(pipe.commands->argv[0], "exit") == 0)
            break;

        if (strcmp(pipe.commands->argv[0], "cd") == 0) {
            if (pipe.commands->argv[1] == NULL) {
                printf("cd: missing argument\n");
                continue;
            }

            if (chdir(pipe.commands->argv[1]) == -1) {
                perror("cd");
            }
            continue;
        }
        execute_pipeline(&pipe);
    }
    return 0;
}