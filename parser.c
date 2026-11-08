#include "parser.h"
#include <stdio.h>
#include <string.h>

static int is_operator(const char *str) {
    return (!strcmp(str, ">") || !strcmp(str, ">>") || !strcmp(str, "<") || !strcmp(str, "|"));
}

int parse_pipeline(char *buffer, pipeline *pipeline) {
    char *p = buffer;
    int token_count = 0;
    int command_index = 0;
    int arg_index = 0;

    for (int i = 0; i < MAX_COMMANDS; i++) {
        pipeline->commands[i].input_file = NULL;
        pipeline->commands[i].output_file = NULL;
        pipeline->commands[i].append_mode = 0;
        pipeline->commands[i].argv[0] = NULL;
    }
    pipeline->command_count = 0;
    while (*p != '\0') {

        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (*p == '\0') {
            break;
        }
        if (token_count >= MAX_ARGS - 1) {
            fprintf(stderr, "shall: too many tokens\n");
            return -1;
        }
        pipeline->commands[0].argv[token_count++] = p;

        while (*p != '\0' && *p != ' ' && *p != '\t') {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    pipeline->commands[0].argv[token_count] = NULL;

    char *tokens[MAX_ARGS];
    for (int i = 0; i <= token_count; i++) {
        tokens[i] = pipeline->commands[0].argv[i];
    }

    if (token_count == 0)
        return 0;

    for (int i = 0; i < token_count; i++) {

        if (strcmp(tokens[i], ">") == 0) {

            if (arg_index == 0) {
                fprintf(stderr, "shall: expected command before >\n");
                return -1;
            }

            if (i + 1 >= token_count || is_operator(tokens[i + 1])) {
                fprintf(stderr, "shall: expected filename after >\n");
                return -1;
            }

            pipeline->commands[command_index].append_mode = 0;
            pipeline->commands[command_index].output_file = tokens[++i];

            continue;
        }

        else if (strcmp(tokens[i], ">>") == 0) {

            if (arg_index == 0) {
                fprintf(stderr, "shall: expected command before >>\n");
                return -1;
            }

            if (i + 1 >= token_count || is_operator(tokens[i + 1])) {
                fprintf(stderr, "shall: expected filename after >>\n");
                return -1;
            }
            pipeline->commands[command_index].append_mode = 1;
            pipeline->commands[command_index].output_file = tokens[++i];

            continue;
        }

        else if (strcmp(tokens[i], "<") == 0) {

            if (arg_index == 0) {
                fprintf(stderr, "shall: expected command before <\n");
                return -1;
            }

            if (i + 1 >= token_count || is_operator(tokens[i + 1])) {
                fprintf(stderr, "shall: expected filename after <\n");
                return -1;
            }
            pipeline->commands[command_index].input_file = tokens[++i];

            continue;
        }

        else if (strcmp(tokens[i], "|") == 0) {

            if (arg_index == 0) {
                fprintf(stderr, "shall: expected command before |\n");
                return -1;
            }

            if (i + 1 >= token_count || is_operator(tokens[i + 1])) {
                fprintf(stderr, "shall: expected command after |\n");
                return -1;
            }

            pipeline->commands[command_index].argv[arg_index] = NULL;
            if (command_index >= MAX_COMMANDS - 1) {
                fprintf(stderr, "shall: too many commands in pipeline\n");
                return -1;
            }
            command_index++;
            arg_index = 0;
            continue;
        } else {
            if (arg_index >= MAX_ARGS - 1) {
                fprintf(stderr, "shall: too many arguments\n");
                return -1;
            }
            pipeline->commands[command_index].argv[arg_index++] = tokens[i];
        }
    }
    pipeline->commands[command_index].argv[arg_index] = NULL;
    pipeline->command_count = command_index + 1;

    return 1;
}
