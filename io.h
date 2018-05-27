#ifndef __IO_H__
#define __IO_H__
#include <stdio.h>
#include <stdbool.h>
#include "parse.h"

typedef enum invocation_type {
    INTERACTIVE = 0,
    SCRIPTED,
    PIPE
} invocation_type;

struct arguments {
    char *prompt;
    invocation_type mode;
    unsigned int seed;
    bool seed_set;
    FILE *ist;
};

void getline_wrapper(struct parse_tree *t, struct arguments *args);
void no_read(struct parse_tree *t, struct arguments *args);
void readline_wrapper(struct parse_tree *t, struct arguments *args);
void read_history_wrapper(const char *filename);
void write_history_wrapper(const char *filename);

extern bool break_print_loop;
void sigint_handler(int sig);
#endif // __IO_H__
