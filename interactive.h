#ifndef __INTERACTIVE_H__
#define __INTERACTIVE_H__
#include "dice.h"

void readline_wrapper(struct parse_tree *t, struct arguments *args);
void read_history_wrapper(const char *filename);
void write_history_wrapper(const char *filename);
#endif // __INTERACTIVE_H__
