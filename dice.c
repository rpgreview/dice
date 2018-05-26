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

#include <readline/readline.h>
#include <readline/history.h>
#include <omp.h>

const char *argp_program_version = "Dice 0.7";
const char *argp_program_bug_address = "cryptarch@github";

#include "args.h"
#include "dice.h"
#include "parse.h"
#include "interactive.h"

bool break_print_loop = false;
void sigint_handler(int sig) {
    break_print_loop = true;
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

void parse_tree_reset(struct parse_tree *t) {
    t->suppress = false;
    t->quit = false;
    t->nreps = 1;
    t->ndice = 0;
    t->last_roll = NULL;
    if(t->dice_specs != NULL) {
        dice_reset(t->dice_specs);
        t->dice_specs = NULL;
    }
}

void dice_init(struct roll_encoding *d) {
    d->ndice = 0;
    d->nsides = 0;
    d->dir = pos;
    d->next = NULL;
}

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

void print_parse_tree(const struct parse_tree *t) {
    printf("{suppress: %s", t->suppress ? "true" : "false");
    printf(", quit: %s", t->quit ? "true" : "false");
    printf(", nreps: %ld", t->nreps);
    printf(", ndice: %ld", t->ndice);
    printf(", roll string: ");
    if(t->dice_specs != NULL) {
        print_dice_specs(t->dice_specs);
    }
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

void getline_wrapper(struct parse_tree *t, struct arguments *args) {
    size_t bufsize = 0;
    char *line = NULL;
    errno = 0;
    int getline_retval = getline(&line, &bufsize, args->ist);
    if(line == NULL || line == 0 || feof(args->ist) || errno != 0 || getline_retval < 0) {
        if(errno != 0) {
            printf("Error %d (%s) getting line for reading.\n", errno, strerror(errno));
        }
        t->quit = true;
        free(line);
        return;
    }
    parse(t, line, bufsize);
    free(line);
}

void no_read(struct parse_tree *t, struct arguments *args) {
    printf("Unknown mode. Not reading any lines.\n");
}

int main(int argc, char** argv) {
    struct arguments args;
    args.prompt = "\001\e[0;32m\002dice> \001\e[0m\002";
    if(isatty(fileno(stdin))) {
        args.mode = INTERACTIVE;
    } else {
        args.mode = PIPE;
    }
    args.ist = stdin;

    FILE *rnd_src;
    char rnd_src_path[] = "/dev/urandom";
    rnd_src = fopen(rnd_src_path, "r");
    args.seed_set = false;
    if(rnd_src) {
        int fread_num_items = fread(&args.seed, sizeof(args.seed), 1, rnd_src);
        if(fread_num_items > 0) {
            args.seed_set = true;
        }
        fclose(rnd_src);
    }

    argp_parse(&argp, argc, argv, 0, 0, &args);

    if(!args.seed_set) {
        fprintf(stderr, "Problem opening %s for reading and/or seed not given, falling back to a time-based random seed.\n", rnd_src_path);
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        args.seed = t.tv_nsec * t.tv_sec;
    }

    srandom(args.seed);

    struct parse_tree *t = malloc(sizeof(struct parse_tree));
    if(!t) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    memset(t, 0, sizeof(struct parse_tree));
    parse_tree_reset(t);

    char *histfile="~/.dice_history";
    void (*process_next_line)(struct parse_tree*, struct arguments*);
    switch(args.mode) {
        case INTERACTIVE:
            {
                read_history_wrapper(histfile);
                rl_bind_key('\t', rl_insert); // File completion is not relevant for this program
                process_next_line = &readline_wrapper;
            }
            break;
        case PIPE: case SCRIPTED:
            {
                process_next_line = &getline_wrapper;
            }
            break;
        default:
            process_next_line = &no_read;
    }

    do {
        process_next_line(t, &args);
    } while(!(t->quit || feof(args.ist)));
    if(args.mode == INTERACTIVE) {
        write_history_wrapper(histfile);
    }
    if(t) {
        parse_tree_reset(t);
        free(t);
        t = NULL;
    }
    return 0;
}
