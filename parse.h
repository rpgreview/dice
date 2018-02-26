#ifndef __PARSE_H__
#define __PARSE_H__
#include <stdlib.h>
#include "dice.h"

typedef enum token_t {
    none = 0,
    number,
    operator,
    command,
    end
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
    operator_pending,   // Always number
    recv_x,             // Always operator 'x'
    recv_num_dice,      // Always number
    recv_d,             // Always operator 'd'/'D'
    recv_num_sides,     // Always number
    recv_shift_dir,     // Always operator '+'/'-'
    recv_shift_amount,  // Always number
    recv_cmd,           // Always string
    finish
} state_t;

void clear_screen();
void token_init(struct token *t);
int lex(struct token *t, int *tokens_found, const char *restrict buf, const size_t len);
void print_state_name(const state_t s);
int parse(struct roll_encoding *restrict d, const char *restrict buf, const size_t len);
#endif // __PARSE_H__
