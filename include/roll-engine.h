#ifndef __ROLL_ENGINE_H__
#define __ROLL_ENGINE_H__
#include "parse.h"

void dice_reset(struct roll_encoding *);
void dice_init(struct roll_encoding *);
void roll(long **results, const struct parse_tree *);
void print_dice_specs(const struct roll_encoding *d);
#endif // __ROLL_ENGINE_H__
