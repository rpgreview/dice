#ifndef __DICE_H__
#define __DICE_H__
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

struct roll_encoding {
    long nreps;
    long ndice;
    long nsides;
    long shift;
    bool suppress;
    bool quit;
};

void dice_init(struct roll_encoding *);
void roll(const struct roll_encoding *d);
#endif // __DICE_H__
