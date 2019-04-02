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

#include "parse.h"
#include "io.h"
#include "roll-engine.h"

void print_dice_specs(const struct roll_encoding *d) {
    switch(d->dir) {
        case pos:
            fprintf(stderr, "+");
            break;
        case neg:
            fprintf(stderr, "-");
            break;
        default:
            fprintf(stderr, "?");
    }
    fprintf(stderr, "%ldd%ld%s", d->ndice, d->nsides, d->explode ? "!" : "");
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
    d->explode = false;
    d->next = NULL;
}

double runif() {
    return (double)random()/RAND_MAX;
}

long single_dice_outcome(long sides, bool explode) {
    if(sides < 1) {
        fprintf(stderr, "Invalid number of sides: %ld\n", sides);
        return LONG_MIN;
    } else if(sides == 1) {
        return 1;
    } else {
        long roll = ceil(runif()*sides);
        long sum = roll;
        if(explode) {
            while(roll == sides) {
                roll = ceil(runif()*sides);
                sum += roll;
            }
        }
        return sum;
    }
}

long parallelised_total_dice_outcome(long sides, long ndice, bool explode) {
    if(sides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return ndice;
    }
    long roll_num = 0;
    long sum = 0;
    bool keep_going = true;
    #pragma omp for schedule(static) private(roll_num) nowait
    for(roll_num = 0; roll_num < ndice; ++roll_num) {
        if(keep_going) {
            sum += single_dice_outcome(sides, explode);
        }
        if(break_print_loop) {
            keep_going = false;
        }
    }
    return sum;
}

long serial_total_dice_outcome(long sides, long ndice, bool explode) {
    if(sides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return ndice;
    }
    long roll_num = 0;
    long sum = 0;
    for(roll_num = 0; roll_num < ndice; ++roll_num) {
        sum += single_dice_outcome(sides, explode);
        if(break_print_loop) {
            break;
        }
    }
    return sum;
}

void check_roll_sanity(const struct parse_tree* t) {
    signal(SIGINT, sigint_handler);
    break_print_loop = false;
    if(t->dice_specs == NULL) {
        return;
    }
    struct roll_encoding *d = t->dice_specs;
    long cumulative_dice_range[] = { 0, 0 };
    bool continuing = true;
    bool warning = false;
    while(continuing) {
        switch(d->dir) {
            case pos:
                if(cumulative_dice_range[1] > 0 && (LONG_MAX-cumulative_dice_range[1])/d->ndice < d->nsides) {
                    warning = true;
                } else {
                    cumulative_dice_range[0] += d->nsides;
                    cumulative_dice_range[1] += d->ndice*d->nsides;
                }
                break;
            case neg:
                if(cumulative_dice_range[1] < 0 && (LONG_MIN-cumulative_dice_range[0])/d->ndice > -d->nsides) {
                    warning = true;
                } else {
                    cumulative_dice_range[0] -= d->ndice*d->nsides;
                    cumulative_dice_range[1] -= d->nsides;
                }
                break;
            default: {}
        }
        if(d->next != NULL && !warning && !break_print_loop) {
            d = d->next;
        } else {
            continuing = false;
        }
    }
    if(warning) {
        fprintf(stderr, "Warning: ");
        print_dice_specs(t->dice_specs);
        fprintf(stderr, " are prone to integer overflow.\n");
    }
}

void parallelised_rep_rolls(const struct parse_tree *t) {
    if(t->dice_specs == NULL) {
        return;
    }
    long rep = 0;
    long nsuccess = 0;
    bool keep_going = true;
    #pragma omp for schedule(static) private(rep) nowait
    for(rep = 0; rep < t->nreps; ++rep) {
        if(rep != 0 && !t->use_threshold) {
            printf(" ");
        }
        struct roll_encoding *d = t->dice_specs;
        long result = 0;
        while(d != NULL && keep_going) {
            if(d->ndice > 0 && d->nsides > 0) {
                result += d->dir * serial_total_dice_outcome(d->nsides, d->ndice, d->explode);
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
        if(t->use_threshold) {
            nsuccess += result >= t->threshold;
        } else {
            printf("%ld", result);
        }
    }
    if(t->use_threshold) {
        printf("%ld", nsuccess);
    }
}

void serial_rep_rolls(const struct parse_tree *t) {
    if(t->dice_specs == NULL) {
        return;
    }
    long rep = 0;
    long nsuccess = 0;
    for(rep = 0; rep < t->nreps; ++rep) {
        if(rep != 0 && !t->use_threshold) {
            printf(" ");
        }
        struct roll_encoding *d = t->dice_specs;
        long result = 0;
        while(d != NULL) {
            if(d->ndice > 0 && d->nsides > 0) {
                result += d->dir * parallelised_total_dice_outcome(d->nsides, d->ndice, d->explode);
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
        if(t->use_threshold) {
            nsuccess += result >= t->threshold;
        } else {
            printf("%ld", result);
        }
    }
    if(t->use_threshold) {
        printf("%ld", nsuccess);
    }
}

void roll(const struct parse_tree *t) {
    signal(SIGINT, sigint_handler);
    break_print_loop = false;
    check_roll_sanity(t);
    if(t->nreps > t->ndice) {
        parallelised_rep_rolls(t);
    } else {
        serial_rep_rolls(t);
    }
    printf("\n");
    #pragma omp flush
}
