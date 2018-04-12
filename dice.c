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

const char *argp_program_version = "Dice 0.6";
const char *argp_program_bug_address = "cryptarch@github";

#include "args.h"
#include "dice.h"
#include "parse.h"
#include "interactive.h"

bool break_print_loop = false;
void sigint_handler(int sig) {
    break_print_loop = true;
}

void dice_init(struct roll_encoding *restrict d) {
    d->nreps = 1;
    d->ndice = 0;
    d->nsides = 0;
    d->shift = 0;
    d->suppress = false;
    d->quit = false;
}

double runif() {
    return (double)random()/RAND_MAX;
}

long single_dice_outcome(long sides) {
    return ceil(runif()*sides);
}

long parallelised_total_dice_outcome(long sides, long ndice) {
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

void parallelised_rep_rolls(const struct roll_encoding *restrict d) {
    long rep = 0;
    bool keep_going = true;
    #pragma omp for schedule(static) private(rep) nowait
    for(rep = 0; rep < d->nreps; ++rep) {
        long result = d->shift;
        if(keep_going) {
            result += serial_total_dice_outcome(d->nsides, d->ndice);
            if(rep != 0) {
                printf(" ");
            }
            printf("%ld", result);
        }
        if(break_print_loop) {
            keep_going = false;
        }
    }
}

void serial_rep_rolls(const struct roll_encoding *restrict d) {
    long rep = 0;
    for(rep = 0; rep < d->nreps; ++rep) {
        long result = d->shift + parallelised_total_dice_outcome(d->nsides, d->ndice);
        if(break_print_loop) {
            break;
        }
        if(rep != 0) {
            printf(" ");
        }
        printf("%ld", result);
    }
}

void roll(const struct roll_encoding *restrict d) {
    signal(SIGINT, sigint_handler);
    break_print_loop = false;
    if(LONG_MAX/d->ndice < d->nsides) {
        fprintf(stderr, "Warning: %ldd%ld dice are prone to integer overflow.\n", d->ndice, d->nsides);
    } else if(d->shift > 0 && LONG_MAX - d->shift < d->ndice*d->nsides) {
        fprintf(stderr, "Warning: %ldd%ld + %ld dice are prone to integer overflow.\n", d->ndice, d->nsides, d->shift);
    }
    if(d->nreps > d->ndice) {
        parallelised_rep_rolls(d);
    } else {
        serial_rep_rolls(d);
    }
    printf("\n");
    #pragma omp flush
}

void getline_wrapper(struct roll_encoding *restrict d, struct arguments *args) {
    size_t bufsize = 0;
    char *line = NULL;
    errno = 0;
    int getline_retval = getline(&line, &bufsize, args->ist);
    if(line == NULL || line == 0 || feof(args->ist) || errno != 0 || getline_retval < 0) {
        if(errno != 0) {
            printf("Error %d (%s) getting line for reading.\n", errno, strerror(errno));
        }
        d->quit = true;
        free(line);
        return;
    }
    parse(d, line, bufsize);
    free(line);
}

void no_read(struct roll_encoding *restrict d, struct arguments *args) {
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

    struct roll_encoding *d = malloc(sizeof(struct roll_encoding));
    if(!d) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    dice_init(d);

    char *histfile="~/.dice_history";
    void (*process_next_line)(struct roll_encoding*, struct arguments*);
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
        process_next_line(d, &args);
    } while(!(d->quit || feof(args.ist)));
    if(args.mode == INTERACTIVE) {
        write_history_wrapper(histfile);
    }
    if(d) {
        free(d);
        d = NULL;
    }
    return 0;
}
