#include "executor.h"
#include "parser.h"
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER 10000
#define HOST_COLOR "\033[36m"  // cyan
#define PATH_COLOR "\033[34m"  // blue
#define SHELL_COLOR "\033[35m" // magenta
#define RESET_COLOR "\033[0m"  // white

static volatile sig_atomic_t sigint_received = 0;

static void handle_sigint(int signum) {
    (void)signum;
    sigint_received = 1;
}
static int readline_event(void) {
    if (!sigint_received)
        return 0;
    rl_replace_line("", 0);
    fputs("^C", rl_outstream);
    rl_done = 1;
    return 0;
}
int main(void) {
    char path[MAX_BUFFER];
    char host_name[MAX_BUFFER];
    char *prompt;
    char status_str[16];
    int needed = 0;
    size_t prompt_size = 0;
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    rl_catch_signals = 0;
    rl_event_hook = readline_event;
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw == NULL) {
        fprintf(stderr, "shall: could not get home directory\n");
        return 1;
    }

    char history_path[MAX_BUFFER];

    int result = snprintf(history_path, sizeof(history_path), "%s/.shall_history", pw->pw_dir);

    if (result < 0 || (size_t)result >= sizeof(history_path)) {
        fprintf(stderr, "shall: history path too long\n");
        return 1;
    }
    using_history();
    read_history(history_path);
    int last_status = 0;
    while (1) {
        struct passwd *user;

        if (getcwd(path, sizeof(path)) == NULL) {
            perror("getcwd");

            needed = snprintf(NULL, 0, "shall: ");
            if (needed < 0) {
                fprintf(stderr, "shall: prompt formatting failed\n");
                break;
            }

            prompt_size = (size_t)needed + 1;
            prompt = malloc(prompt_size);

            if (prompt == NULL) {
                perror("malloc");
                break;
            }

            snprintf(prompt, prompt_size, "shall: ");

        } else if (gethostname(host_name, sizeof(host_name)) == -1) {
            perror("gethostname");

            needed = snprintf(NULL, 0, "shall: ");
            if (needed < 0) {
                fprintf(stderr, "shall: prompt formatting failed\n");
                break;
            }

            prompt_size = (size_t)needed + 1;
            prompt = malloc(prompt_size);

            if (prompt == NULL) {
                perror("malloc");
                break;
            }

            snprintf(prompt, prompt_size, "shall: ");

        } else if ((user = getpwuid(getuid())) == NULL) {
            fprintf(stderr, "shall: could not get username\n");

            needed = snprintf(NULL, 0, "shall: ");
            if (needed < 0) {
                fprintf(stderr, "shall: prompt formatting failed\n");
                break;
            }

            prompt_size = (size_t)needed + 1;
            prompt = malloc(prompt_size);

            if (prompt == NULL) {
                perror("malloc");
                break;
            }

            snprintf(prompt, prompt_size, "shall: ");

        } else {
            size_t len = strlen(user->pw_dir);

            if (strncmp(path, user->pw_dir, len) == 0 && (path[len] == '/' || path[len] == '\0')) {

                needed =
                    snprintf(NULL, 0, "shall: %s@%s:~%s %% ", user->pw_name, host_name, path + len);

                if (needed < 0) {
                    fprintf(stderr, "shall: prompt formatting failed\n");
                    break;
                }

                prompt_size = (size_t)needed + 1;
                prompt = malloc(prompt_size);

                if (prompt == NULL) {
                    perror("malloc");
                    break;
                }

                snprintf(prompt, prompt_size, "shall: %s@%s:~%s %% ", user->pw_name, host_name,
                         path + len);

            } else {
                needed = snprintf(NULL, 0, "shall: %s@%s:%s %% ", user->pw_name, host_name, path);

                if (needed < 0) {
                    fprintf(stderr, "shall: prompt formatting failed\n");
                    break;
                }

                prompt_size = (size_t)needed + 1;
                prompt = malloc(prompt_size);

                if (prompt == NULL) {
                    perror("malloc");
                    break;
                }

                snprintf(prompt, prompt_size, "shall: %s@%s:%s %% ", user->pw_name, host_name,
                         path);
            }
        }
        sigint_received = 0;
        char *buffer = readline(prompt);
        free(prompt);
        if (sigint_received) {
            last_status = 130;
            sigint_received = 0;
            free(buffer);
            continue;
        }
        if (buffer == NULL)
            break;

        if (buffer[strspn(buffer, " \t")] != '\0') {
            add_history(buffer);
            int history_result = write_history(history_path);
            if (history_result != 0) {
                fprintf(stderr, "shall: history: %s\n", strerror(history_result));
            }
        }

        pipeline pipe = {0};
        int parse_result = parse_pipeline(buffer, &pipe);

        if (parse_result < 0) {
            last_status = 2;
            free(buffer);
            continue;
        }
        if (parse_result == 0) {
            free(buffer);
            continue;
        }
        for (int i = 0; i < pipe.command_count; i++) {
            for (int j = 0; pipe.commands[i].argv[j] != NULL; j++) {
                if (strcmp(pipe.commands[i].argv[j], "$?") == 0) {
                    snprintf(status_str, sizeof(status_str), "%d", last_status);
                    pipe.commands[i].argv[j] = status_str;
                }
            }
        }
        if (strcmp(pipe.commands->argv[0], "exit") == 0) {
            free(buffer);
            break;
        }

        if (strcmp(pipe.commands->argv[0], "cd") == 0) {
            if (pipe.commands->argv[1] == NULL) {
                printf("cd: missing argument\n");
                last_status = 1;
                free(buffer);
                continue;
            }

            if (chdir(pipe.commands->argv[1]) == -1) {
                last_status = 1;
                perror("cd");
            } else {
                last_status = 0;
            }
            free(buffer);
            continue;
        }
        last_status = execute_pipeline(&pipe);
        free(buffer);
    }

    return last_status;
}