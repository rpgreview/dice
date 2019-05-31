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
#include "util.h"

bool break_print_loop = false;

void sigint_handler(int sig) {
    break_print_loop = true;
}

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
    d->keep = false;
    d->discard = 0;
    d->next = NULL;
}

double runif() {
    return (double)random()/RAND_MAX;
}

long single_dice_outcome(struct roll_encoding *d) {
    if(d->nsides < 1) {
        fprintf(stderr, "Invalid number of sides: %ld\n", d->nsides);
        return LONG_MIN;
    } else if(d->nsides == 1) {
        return 1;
    } else {
        long roll = ceil(runif()*d->nsides);
        long sum = roll;
        if(d->explode) {
            while(roll == d->nsides) {
                roll = ceil(runif()*d->nsides);
                sum += roll;
            }
        }
        return sum;
    }
}

long parallelised_total_dice_outcome(struct roll_encoding *d) {
    if(d->nsides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return d->ndice;
    }
    long sum = 0;
    bool keep_going = true;
    long *rolls = malloc(sizeof(long)*d->ndice);
    long roll_num;
    #pragma omp parallel for private(roll_num) shared(keep_going, rolls)
    for(roll_num = 0; roll_num < d->ndice && keep_going; ++roll_num) {
        if(keep_going) {
            rolls[roll_num] = single_dice_outcome(d);
            sum += rolls[roll_num];
        }
        if(break_print_loop) {
            keep_going = false;
        }
    }
    if(d->discard > 0) {
        qsort_r(rolls, d->ndice, sizeof(long), integer_difference_sign, NULL);
        long remove = 0;
        for(roll_num = 0; roll_num < d->ndice && roll_num < d->discard; ++roll_num) {
            remove += rolls[roll_num];
        }
        sum -= remove;
    }
    free(rolls);
    return sum;
}

long serial_total_dice_outcome(struct roll_encoding *d) {
    if(d->nsides == 1) { // Optimise for non-random parts eg the "+1" in "d4+1".
        return d->ndice;
    }
    long roll_num = 0;
    long sum = 0;
    long *rolls = malloc(sizeof(long)*d->ndice);
    for(roll_num = 0; roll_num < d->ndice; ++roll_num) {
        rolls[roll_num] = single_dice_outcome(d);
        sum += rolls[roll_num];
        if(break_print_loop) {
            break;
        }
    }
    if(d->discard > 0) {
        qsort_r(rolls, d->ndice, sizeof(long), integer_difference_sign, NULL);
        long remove = 0;
        for(roll_num = 0; roll_num < d->ndice && roll_num < d->discard; ++roll_num) {
            remove += rolls[roll_num];
        }
        sum -= remove;
    }
    free(rolls);
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
        long ndice = d->ndice;
        if(d->keep) {
            ndice = d->ndice - d->discard < 0 ? 0 : d->ndice - d->discard;
        }
        switch(d->dir) {
            case pos:
                if(cumulative_dice_range[1] > 0 && ndice > 0 && (LONG_MAX-cumulative_dice_range[1])/d->ndice < d->nsides) {
                    warning = true;
                } else {
                    cumulative_dice_range[0] += d->nsides;
                    cumulative_dice_range[1] += ndice*d->nsides;
                }
                break;
            case neg:
                if(cumulative_dice_range[1] < 0 && ndice > 0 && (LONG_MIN-cumulative_dice_range[0])/d->ndice > -d->nsides) {
                    warning = true;
                } else {
                    cumulative_dice_range[0] -= ndice*d->nsides;
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
    #pragma omp parallel for private(rep) shared(keep_going)
    for(rep = 0; rep < t->nreps; ++rep) {
        if(rep != 0 && !t->use_threshold) {
            printf(" ");
        }
        struct roll_encoding *d = t->dice_specs;
        long result = 0;
        while(d != NULL && keep_going) {
            if(d->ndice > 0 && d->nsides > 0) {
                result += d->dir * serial_total_dice_outcome(d);
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
                result += d->dir * parallelised_total_dice_outcome(d);
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
