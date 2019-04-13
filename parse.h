#ifndef __PARSE_H__
#define __PARSE_H__
#include <stdlib.h>
#include <stdbool.h>

#define LONG_MAX_STR_LEN 19 // Based on decimal representation of LONG_MAX

typedef enum direction {
    neg = -1,
    pos = 1
} direction;

struct roll_encoding;
struct roll_encoding {
    long ndice;
    long nsides;
    direction dir;
    bool explode;
    bool keep;
    long discard;
    struct roll_encoding *next;
};

struct parse_tree;
struct parse_tree {
    bool suppress; // Used to silence output, eg when clearing screen
    bool quit;
    long nreps;
    bool use_threshold;
    long threshold;
    struct roll_encoding *dice_specs;
    struct roll_encoding *last_roll; // Easily find latest entry in dice_specs list
    long ndice;
    struct parse_tree *next;
    struct parse_tree *current;
};

typedef enum token_t {
    none = 0,
    number,
    dice_operator,
    rep_operator,
    additive_operator,
    explode_operator,
    threshold_operator,
    keep_operator,
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
    check_modifiers_or_more_rolls,
    check_more_rolls,
    want_threshold,
    want_keep_number,
    check_end,
    finish
} state_t;

void clear_screen();
void token_init(struct token *t);
int lex(struct token *t, int *tokens_found, const char *buf, const size_t len);
void print_state_name(const state_t s);
void print_parse_tree(const struct parse_tree *t);
int parse(struct parse_tree *t, const char *buf, const size_t len);
void parse_tree_initialise(struct parse_tree *t);
void parse_tree_reset(struct parse_tree *t);
#endif // __PARSE_H__
