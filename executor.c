#include "executor.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <sys/wait.h>
#include <unistd.h>

static void apply_redirections(const command *cmd);
static noreturn void exec_command(const command *cmd);

int execute_pipeline(const pipeline *pipeline) {
    if (pipeline->command_count > 1) {
        int pipe_count = pipeline->command_count - 1;
        int command_count = pipeline->command_count;
        int pipefd[MAX_COMMANDS - 1][2];
        for (int i = 0; i < pipe_count; i++) {
            if (pipe(pipefd[i]) == -1) {
                perror("pipe");
                for (int j = 0; j < i; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                return -1;
            }
        }
        pid_t pids[MAX_COMMANDS];
        for (int i = 0; i < command_count; i++) {
            pids[i] = fork();
            if (pids[i] == -1) {
                perror("fork");
                for (int j = 0; j < pipe_count; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                for (int j = 0; j < i; j++) {
                    if (waitpid(pids[j], NULL, 0) == -1)
                        perror("waitpid");
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
                if (i < pipe_count) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        _exit(1);
                    }
                }
                apply_redirections(&pipeline->commands[i]);
                for (int j = 0; j < pipe_count; j++) {
                    close(pipefd[j][0]);
                    close(pipefd[j][1]);
                }
                exec_command(&pipeline->commands[i]);
            }
        }
        for (int i = 0; i < pipe_count; i++) {
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
        if (waitpid(pid, NULL, 0) == -1) {
            perror("waitpid");
        }
    } else if (pid == 0) {
        apply_redirections(&pipeline->commands[0]);
        exec_command(&pipeline->commands[0]);
    } else {
        perror("fork");
        return -1;
    }
    return 1;
}

static void apply_redirections(const command *cmd) {
    if (cmd->input_file != NULL) {
        int file_fd = open(cmd->input_file, O_RDONLY);
        if (file_fd == -1) {
            perror(cmd->input_file);
            _exit(1);
        }
        if (dup2(file_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(file_fd);
            _exit(1);
        }
        close(file_fd);
    }

    if (cmd->output_file != NULL) {
        int file_fd = cmd->append_mode ? open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
                                       : open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (file_fd == -1) {
            perror(cmd->output_file);
            _exit(1);
        }
        if (dup2(file_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(file_fd);
            _exit(1);
        }
        close(file_fd);
    }
}

static noreturn void exec_command(const command *cmd) {
    execvp(cmd->argv[0], cmd->argv);

    if (errno == ENOENT) {
        fprintf(stderr, "shall: command not found: %s\n", cmd->argv[0]);
    } else {
        perror(cmd->argv[0]);
    }

    _exit(127);
}