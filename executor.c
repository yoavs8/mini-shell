#include "executor.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_pipeline(const pipeline *pipeline) {
    int file_fd = -1;
    if (pipeline->command_count > 1) {
        int pipe_count = pipeline->command_count - 1;
        int command_count = pipeline->command_count;
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < pipe_count; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                return -1;
            }
        }
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] == -1) {
                perror("fork");
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                return -1;
            }
            if (pids[i] == 0) {
                if (i > 0) {
                    if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1) {
                        perror("dup2");
                        _exit(1);
                    }
                }
                if (i < command_count - 1) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        _exit(1);
                    }
                }
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                execvp(pipeline->commands[i].argv[0], pipeline->commands[i].argv);
                if (errno == ENOENT) {
                    fprintf(stderr, "shall: command not found: %s\n",
                            pipeline->commands[i].argv[0]);
                } else {
                    perror(pipeline->commands[i].argv[0]);
                }
                _exit(127);
            }
            continue;
        }
        for (int i = 0; i < command_count - 1; i++) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        for (int i = 0; i < command_count; i++) {
            if (waitpid(pids[i], NULL, 0) == -1)
                perror("waitpid");
        }
        return 1;
    }
    pid_t pid = fork();
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else if (pid == 0) {
        if (pipeline->commands->output_file != NULL) {
            file_fd =
                pipeline->commands->append_mode
                    ? open(pipeline->commands->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
                    : open(pipeline->commands->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (file_fd == -1) {
                perror(pipeline->commands->output_file);
                _exit(1);
            }
            if (dup2(file_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                close(file_fd);
                _exit(1);
            }
            close(file_fd);
        }

        if (pipeline->commands->input_file != NULL) {
            file_fd = open(pipeline->commands->input_file, O_RDONLY);
            if (file_fd == -1) {
                perror(pipeline->commands->input_file);
                _exit(1);
            }
            if (dup2(file_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                close(file_fd);
                _exit(1);
            }
            close(file_fd);
        }

        execvp(pipeline->commands[0].argv[0], pipeline->commands[0].argv);
        if (errno == ENOENT) {
            fprintf(stderr, "shall: command not found: %s\n", pipeline->commands[0].argv[0]);
        } else {
            perror(pipeline->commands[0].argv[0]);
        }
        _exit(1);
    } else {
        perror("fork");
        return -1;
    }
    return 1;
}