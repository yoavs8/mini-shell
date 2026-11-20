#include "executor.h"
#include "parser.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
        int status;
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
                    if (pids[j] > 0)
                        kill(pids[j], SIGKILL);
                }

                for (int j = 0; j < i; j++) {
                    while (waitpid(pids[j], &status, 0) == -1) {
                        if (errno == EINTR)
                            continue;

                        perror("waitpid");
                        break;
                    }
                }

                return -1;
            }
            if (pids[i] == 0) {
                struct sigaction sa_default = {0};
                sa_default.sa_handler = SIG_DFL;
                if (sigemptyset(&sa_default.sa_mask) == -1) {
                    perror("sigemptyset");
                    _exit(EXIT_FAILURE);
                }
                if (sigaction(SIGINT, &sa_default, NULL) == -1) {
                    perror("sigaction");
                    _exit(EXIT_FAILURE);
                }
                if (i > 0) {
                    if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1) {
                        perror("dup2");
                        _exit(EXIT_FAILURE);
                    }
                }
                if (i < pipe_count) {
                    if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        _exit(EXIT_FAILURE);
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
        int last_status = 0;
        int got_last_status = 0;

        for (int i = 0; i < command_count; i++) {
            pid_t result;

            do {
                result = waitpid(pids[i], &status, 0);
            } while (result == -1 && errno == EINTR);

            if (result == -1) {
                perror("waitpid");
                continue;
            }

            if (i == command_count - 1) {
                last_status = status;
                got_last_status = 1;
            }
        }
        if (!got_last_status)
            return -1;

        if (WIFEXITED(last_status))
            return WEXITSTATUS(last_status);

        if (WIFSIGNALED(last_status))
            return 128 + WTERMSIG(last_status);

        return -1;
    }

    pid_t pid = fork();
    int status;
    if (pid > 0) {
        pid_t result;

        do {
            result = waitpid(pid, &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status))
            return WEXITSTATUS(status);

        if (WIFSIGNALED(status))
            return 128 + WTERMSIG(status);

        return -1;
    } else if (pid == 0) {
        struct sigaction sa_default = {0};
        sa_default.sa_handler = SIG_DFL;
        if (sigemptyset(&sa_default.sa_mask) == -1) {
            perror("sigemptyset");
            _exit(EXIT_FAILURE);
        }
        if (sigaction(SIGINT, &sa_default, NULL) == -1) {
            perror("sigaction");
            _exit(EXIT_FAILURE);
        }
        apply_redirections(&pipeline->commands[0]);
        exec_command(&pipeline->commands[0]);
    } else {
        perror("fork");
        return -1;
    }
}

static void apply_redirections(const command *cmd) {
    if (cmd->input_file != NULL) {
        int file_fd = open(cmd->input_file, O_RDONLY);
        if (file_fd == -1) {
            perror(cmd->input_file);
            _exit(EXIT_FAILURE);
        }
        if (dup2(file_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(file_fd);
            _exit(EXIT_FAILURE);
        }
        close(file_fd);
    }

    if (cmd->output_file != NULL) {
        int file_fd = cmd->append_mode ? open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
                                       : open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (file_fd == -1) {
            perror(cmd->output_file);
            _exit(EXIT_FAILURE);
        }
        if (dup2(file_fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(file_fd);
            _exit(EXIT_FAILURE);
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