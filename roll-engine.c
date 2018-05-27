#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <limits.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <omp.h>

#include "dice.h"
#include "parse.h"
#include "io.h"
#include "roll-engine.h"

void print_dice_specs(const struct roll_encoding *d) {
    switch(d->dir) {
        case pos:
            printf("+");
            break;
        case neg:
            printf("-");
            break;
        default:
            printf("?");
    }
    printf("%ldd%ld", d->ndice, d->nsides);
    if(d->next != NULL) {
        print_dice_specs(d->next);
    }
}

void free_dice(struct roll_encoding *);
void free_dice(struct roll_encoding *d) {
    if(d->next != NULL) {
        free_dice(d->next);
    }
    dice_init(d);
    free(d);
}

void dice_reset(struct roll_encoding *d) {
    if(d->next != NULL) {
        dice_reset(d->next);
    }
    dice_init(d);
    free(d);
}

void dice_init(struct roll_encoding *d) {
    d->ndice = 0;
    d->nsides = 0;
    d->dir = pos;
    d->next = NULL;
}

double runif() {
    return (double)random()/RAND_MAX;
}

long single_dice_outcome(long sides) {
    if(sides < 1) {
        fprintf(stderr, "Invalid number of sides: %ld\n", sides);
        return LONG_MIN;
    } else if(sides == 1) {
        return 1;
    } else {
        return ceil(runif()*sides);
    }
}

long parallelised_total_dice_outcome(long sides, long ndice) {
    if(sides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return ndice;
    }
    long roll_num = 0;
    long sum = 0;
    bool keep_going = true;
    #pragma omp for schedule(static) private(roll_num) nowait
    for(roll_num = 0; roll_num < ndice; ++roll_num) {
        if(keep_going) {
            sum += single_dice_outcome(sides);
        }
        if(break_print_loop) {
            keep_going = false;
        }
    }
    return sum;
}

long serial_total_dice_outcome(long sides, long ndice) {
    if(sides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return ndice;
    }
    long roll_num = 0;
    long sum = 0;
    for(roll_num = 0; roll_num < ndice; ++roll_num) {
        sum += single_dice_outcome(sides);
        if(break_print_loop) {
            break;
        }
    }
    return sum;
}

void check_roll_sanity(const struct roll_encoding* d, const long result_so_far) {
    if(LONG_MAX/d->ndice < d->nsides) {
        fprintf(stderr, "Warning: %ldd%ld dice are prone to integer overflow.\n", d->ndice, d->nsides);
    }
}

void parallelised_rep_rolls(const struct parse_tree *t) {
    long rep = 0;
    bool keep_going = true;
    if(t->dice_specs == NULL) {
        return;
    }
    #pragma omp for schedule(static) private(rep) nowait
    for(rep = 0; rep < t->nreps; ++rep) {
        if(rep != 0) {
            printf(" ");
        }
        struct roll_encoding *d = t->dice_specs;
        long result = 0;
        while(d != NULL && keep_going) {
            if(d->ndice > 0 && d->nsides > 0) {
                check_roll_sanity(d, result);
                result += d->dir * serial_total_dice_outcome(d->nsides, d->ndice);
            }
            if(d->next != NULL) {
                d = d->next;
            } else {
                d = NULL;
            }
            if(break_print_loop) {
                keep_going = false;
            }
        }
        printf("%ld", result);
    }
}

void serial_rep_rolls(const struct parse_tree *t) {
    long rep = 0;
    if(t->dice_specs == NULL) {
        return;
    }
    for(rep = 0; rep < t->nreps; ++rep) {
        if(rep != 0) {
            printf(" ");
        }
        struct roll_encoding *d = t->dice_specs;
        long result = 0;
        while(d != NULL) {
            if(d->ndice > 0 && d->nsides > 0) {
                check_roll_sanity(d, result);
                result += d->dir * parallelised_total_dice_outcome(d->nsides, d->ndice);
            }
            if(d->next != NULL) {
                d = d->next;
            } else {
                d = NULL;
            }
            if(break_print_loop) {
                break;
            }
        }
        printf("%ld", result);
    }
}

void roll(const struct parse_tree *t) {
    signal(SIGINT, sigint_handler);
    break_print_loop = false;
    if(t->nreps > t->ndice) {
        parallelised_rep_rolls(t);
    } else {
        serial_rep_rolls(t);
    }
    printf("\n");
    #pragma omp flush
}
