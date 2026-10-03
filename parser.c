#include "parser.h"
#include <stdio.h>
#include <string.h>

int parse_command(char *buffer, Command *cmd) {
    char *p = buffer;
    int arg_count = 0;

    cmd->right_argv = NULL;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = 0;
    while (*p != '\0') {

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (arg_count < MAX_ARGS - 1) {
            cmd->argv[arg_count++] = p;
        }

        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    cmd->argv[arg_count] = NULL;

    if (arg_count == 0)
        return 0;
    for (int i = 0; i < arg_count; i++) {

        if (strcmp(cmd->argv[i], ">") == 0) {

            if (i == 0) {
                fprintf(stderr, "shall: expected command before >\n");
                return -1;
            }

            if (i + 1 >= arg_count) {
                fprintf(stderr, "shall: expected filename after >\n");
                return -1;
            }

            cmd->output_file = cmd->argv[i + 1];
            cmd->append_mode = 0;

            cmd->argv[i] = NULL;
            i++;
        }

        else if (strcmp(cmd->argv[i], ">>") == 0) {

            if (i == 0) {
                fprintf(stderr, "shall: expected command before >>\n");
                return -1;
            }

            if (i + 1 >= arg_count) {
                fprintf(stderr, "shall: expected filename after >>\n");
                return -1;
            }

            cmd->output_file = cmd->argv[i + 1];
            cmd->append_mode = 1;

            cmd->argv[i] = NULL;
            i++;
        }

        else if (strcmp(cmd->argv[i], "<") == 0) {

            if (i == 0) {
                fprintf(stderr, "shall: expected command before <\n");
                return -1;
            }

            if (i + 1 >= arg_count) {
                fprintf(stderr, "shall: expected filename after <\n");
                return -1;
            }

            cmd->input_file = cmd->argv[i + 1];

            cmd->argv[i] = NULL;
            i++;
        }

        else if (strcmp(cmd->argv[i], "|") == 0) {

            if (i == 0) {
                fprintf(stderr, "shall: expected command before |\n");
                return -1;
            }

            if (i + 1 >= arg_count) {
                fprintf(stderr, "shall: expected command after |\n");
                return -1;
            }

            cmd->right_argv = &cmd->argv[i + 1];

            cmd->argv[i] = NULL;

            break;
        }
    }

    return 1;
}