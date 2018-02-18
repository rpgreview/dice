#ifndef __DICE_H__
#define __DICE_H__
#include <stdbool.h>

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
