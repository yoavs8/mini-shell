#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 64

typedef struct {
    char *argv[MAX_ARGS];
    char **right_argv;
    char *input_file;
    char *output_file;
    int append_mode;
} Command;

int parse_command(char *buffer, Command *cmd);

#endif