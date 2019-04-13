#define _GNU_SOURCE 1 // Needed to avoid various "implicit function declaration" warnings/errors.
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <readline/readline.h>

#include "args.h"
#include "parse.h"
#include "io.h"

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
    parse_tree_initialise(t);

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
