#ifndef __PARSE_H__
#define __PARSE_H__
#include <stdlib.h>
#include "dice.h"

typedef enum token_t {
    none = 0,
    number,
    dice_operator,
    rep_operator,
    additive_operator,
    command,
    statement_delimiter,
    eol
} token_t;

typedef enum cmd_t {
    unknown = -1,
    quit = 0,
    clear
} cmd_t;

struct cmd_map {
    cmd_t cmd_code;
    char cmd_str[8];
};

struct token {
    token_t type;
    long number;
    char op;
    cmd_t cmd;
};

typedef enum state_t {
    error = -1,
    start = 0,
    decide_reps_or_rolls,
    want_number_of_sides,
    check_number_of_dice,
    want_roll,
    check_dice_operator,
    check_more_rolls,
    check_end,
    finish
} state_t;

void clear_screen();
void token_init(struct token *t);
int lex(struct token *t, int *tokens_found, const char *buf, const size_t len);
void print_state_name(const state_t s);
int parse(struct parse_tree *t, const char *buf, const size_t len);
#endif // __PARSE_H__
