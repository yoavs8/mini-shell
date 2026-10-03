#include "executor.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_command(const Command *cmd) {
    int file_fd = -1;
    if (cmd->right_argv != NULL) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            return -1;
        }
        pid_t left_pid = fork();
        if (left_pid == -1) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            return -1;
        }
        if (left_pid == 0) {
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                close(pipefd[0]);
                close(pipefd[1]);
                _exit(1);
            }
            close(pipefd[0]);
            close(pipefd[1]);

            execvp(cmd->argv[0], cmd->argv);
            if (errno == ENOENT) {
                fprintf(stderr, "shall: command not found: %s\n", cmd->argv[0]);
            } else {
                perror(cmd->argv[0]);
            }
            _exit(127);
        }
        pid_t right_pid = fork();

        if (right_pid == -1) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(left_pid, NULL, 0);
            return -1;
        }
        if (right_pid == 0) {
            if (dup2(pipefd[0], STDIN_FILENO) == -1) {
                perror("dup2");
                close(pipefd[0]);
                close(pipefd[1]);
                _exit(1);
            }
            close(pipefd[0]);
            close(pipefd[1]);

            execvp(cmd->right_argv[0], cmd->right_argv);
            if (errno == ENOENT) {
                fprintf(stderr, "shall: command not found: %s\n", cmd->right_argv[0]);
            } else {
                perror(cmd->argv[0]);
            }
            _exit(127);
        }
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(left_pid, NULL, 0);
        waitpid(right_pid, NULL, 0);
        return 1;
    }
    pid_t pid = fork();
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else if (pid == 0) {
        if (cmd->output_file != NULL) {
            file_fd = cmd->append_mode ? open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
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

        if (cmd->input_file != NULL) {
            file_fd = open(cmd->input_file, O_RDONLY);
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

        execvp(cmd->argv[0], cmd->argv);
        if (errno == ENOENT) {
            fprintf(stderr, "shall: command not found: %s\n", cmd->argv[0]);
        } else {
            perror(cmd->argv[0]);
        }
        _exit(1);
    } else {
        perror("fork");
        return -1;
    }
    return 1;
}