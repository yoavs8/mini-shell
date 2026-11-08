#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 64
#define MAX_COMMANDS 64

typedef struct {
    char *argv[MAX_ARGS];
    char **right_argv;
    char *input_file;
    char *output_file;
    int append_mode;
} command;

typedef struct {
    command commands[MAX_COMMANDS];
    int command_count;
} pipeline;

int parse_pipeline(char *buffer, pipeline *pipeline);

#endif