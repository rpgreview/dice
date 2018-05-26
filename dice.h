#ifndef __DICE_H__
#define __DICE_H__
#include <stdio.h>
#include <stdbool.h>

typedef enum invocation_type {
    INTERACTIVE = 0,
    SCRIPTED,
    PIPE
} invocation_type;

/* This structure is used by main to communicate with parse_opt. */
struct arguments {
    char *prompt;
    invocation_type mode;
    unsigned int seed;
    bool seed_set;
    FILE *ist;
};

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
    struct roll_encoding *next;
};

struct parse_tree {
    bool suppress; // Used to silence output, eg when clearing screen
    bool quit;
    long nreps;
    struct roll_encoding *dice_specs;
    struct roll_encoding *last_roll; // Easily find latest entry in dice_specs list
    long ndice;
};

void parse_tree_reset(struct parse_tree*);
void dice_reset(struct roll_encoding *);
void dice_init(struct roll_encoding *);
void roll(const struct parse_tree *);
#endif // __DICE_H__
