#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "args.h"

#define LONG_MAX_STR_LEN 19 // Based on decimal representation of LONG_MAX

const char *argp_program_version = "Dice 0.1";
const char *argp_program_bug_address = "cryptarch@github";

struct roll_encoding {
    long ndice;
    long nsides;
    long shift;
    bool quit;
};

int parse(struct roll_encoding *d, const char *buf, const size_t len) {
    d->ndice = 0;
    d->nsides = 0;
    d->shift = 0;
    d->quit = false;

    int charnum = 0;

    // First eliminate any leading whitespace.
    while(isspace(*(buf + charnum))) {
        ++charnum;
    }

    // The non-whitespace part of buf should begin with 'd' or 'D', optionally prefixed by number of dice (default: 1)
    // Also offer quit command.
    // Also don't complain about empty lines.
    if(*(buf + charnum) == 'd' || *(buf + charnum) == 'D') {
        d->ndice = 1;
    } else if(isdigit(*(buf + charnum))) {
        char ndice_str[LONG_MAX_STR_LEN + 1];
        memset(ndice_str, 0, LONG_MAX_STR_LEN + 1);
        int offset = charnum; // If whitespace was progressed through earlier, charnum is now offset from current digit.
        while(charnum - offset < LONG_MAX_STR_LEN && isdigit(*(buf + charnum))) {
            ndice_str[charnum - offset] = *(buf + charnum);
            ++charnum;
        }
        ndice_str[charnum] = '\0';
        errno = 0;
        d->ndice = strtol(ndice_str, NULL, 10);
        if((errno == ERANGE && (d->ndice == LONG_MAX || d->ndice == LONG_MIN))
            || d->ndice <= 0) {
            printf("Invalid number of dice: %s\n", ndice_str);
            printf("(Require: 0 < number of dice < %ld)\n", LONG_MAX);
            return 1;
        }
    } else if(*(buf + charnum) == 'q') {
        const int cmd_len = strlen("quit");
        char quit_str[cmd_len + 1];
        memset(quit_str, 0, cmd_len + 1);
        int offset = charnum; // If whitespace was progressed through earlier, charnum is now offset from intra-word character place.
        while(charnum - offset < cmd_len && isalpha(*(buf + charnum))) {
            quit_str[charnum - offset] = *(buf + charnum);
            ++charnum;
        }
        quit_str[charnum] = '\0';
        if(0 != strncmp(quit_str, "quit", 5)) {
            printf("\nUnknown command: %s\n", quit_str);
            return 1;
        }
        // Okay, they said quit, but after this there shouldn't be anything else but whitespace.
        while(isspace(*(buf + charnum))) {
            ++charnum;
        }
        if(*(buf + charnum) == '\0') {
            d->quit = true;
            return 1;
        } else {
            printf("Unexpected symbol: %c\n", *(buf + charnum));
            return 1;
        }
    } else if(*(buf + charnum) == '\0') {
        return 1;
    } else {
        printf("\nUnknown symbol in first character: %c (with hex encoding %x)\n", *(buf + charnum), *(buf + charnum));
        return 1;
    }

    // If 'd' was prefixed with some number, there could be some extra whitespace to progress through;
    // also check for invalid entries like "100 E 17".
    while(isspace(*(buf + charnum))) {
        ++charnum;
    }
    if(!(*(buf + charnum) == 'd' || *(buf + charnum) == 'D')) {
        printf("Invalid symbol detected: %c\n", *(buf + charnum));
        return 1;
    } else {
        ++charnum;
    }

    while(isspace(*(buf + charnum))) {
        ++charnum;
    }

    // Grab the number of sides.
    if(isdigit(*(buf + charnum))) {
        char nsides_str[LONG_MAX_STR_LEN + 1];
        memset(nsides_str, 0, LONG_MAX_STR_LEN + 1);
        int offset = charnum;
        while(charnum - offset < LONG_MAX_STR_LEN && isdigit(*(buf + charnum))) {
            nsides_str[charnum - offset] = *(buf + charnum);
            ++charnum;
        }
        nsides_str[charnum] = '\0';
        errno = 0;
        d->nsides = strtol(nsides_str, NULL, 10);
        if((errno == ERANGE && (d->nsides == LONG_MAX || d->nsides == LONG_MIN))
            || d->nsides <= 1) {
            printf("Invalid number of sides: %s\n", nsides_str);
            printf("(Require: 1 < number of dice < %ld)\n", LONG_MAX);
            return 1;
        }
    } else {
        printf("\nUnknown symbol in number of dice specification: %c (with hex encoding %x)\n", *(buf + charnum), *(buf + charnum));
        return 1;
    }

    while(isspace(*(buf + charnum))) {
        ++charnum;
    }

    switch(*(buf + charnum)) {
        case '\0':
            {
                return 0;
            }
            break;
        case '+': case '-':
            {
                int mult = *(buf + charnum) == '-' ? -1 : 1;
                ++charnum;
                while(isspace(*(buf + charnum))) {
                    ++charnum;
                }
                if(isdigit(*(buf + charnum))) {
                    char shift_str[LONG_MAX_STR_LEN + 1];
                    memset(shift_str, 0, LONG_MAX_STR_LEN + 1);
                    int offset = charnum;
                    while(charnum - offset < LONG_MAX_STR_LEN && isdigit(*(buf + charnum))) {
                        shift_str[charnum - offset] = *(buf + charnum);
                        ++charnum;
                    }
                    shift_str[charnum] = '\0';
                    errno = 0;
                    d->shift = strtol(shift_str, NULL, 10);
                    if((errno == ERANGE && (d->shift == LONG_MAX || d->shift == LONG_MIN))
                        || (errno != 0 && d->shift == 0)) {
                        printf("Invalid addition/subtraction value: %s%s\n", mult == -1 ? "-" : "", shift_str);
                        return 1;
                    }
                    d->shift *= mult;
                } else {
                    printf("\nUnknown symbol in number to %s roll: %c (with hex encoding %x)\n", mult == -1 ? "subtract from" : "add to", *(buf + charnum), *(buf + charnum));
                    return 1;
                }
            }
            break;
        default:
            printf("Invalid symbol: %c\n", *(buf + charnum));
            return 1;
    }

    // Everything else should be whitespace until '\0'.
    while(isspace(*(buf + charnum))) {
        ++charnum;
    }
    if(*(buf + charnum) != '\0') {
        printf("Invalid character following dice expression: %c\n", *(buf + charnum));
        return 1;
    }

    return 0;
}

double runif() {
    return (double)random()/RAND_MAX;
}

long single_dice_outcome(long sides) {
    return ceil(runif()*sides);
}

void roll(const struct roll_encoding *d) {
    long result = d->shift;
    int roll_num;
    for(roll_num = 0; roll_num < d->ndice; ++roll_num) {
        result += single_dice_outcome(d->nsides);
    }
    printf("%ld\n", result);
}

void readline_wrapper(struct roll_encoding *d, struct arguments *args) {
    char *line = readline(args->prompt);
    if(line == NULL || line == 0) {
        printf("\n");
        d->quit = true;
        free(line);
        return;
    }
    size_t bufsize = strlen(line);
    if(0 == parse(d, line, bufsize)) {
        roll(d);
        add_history(line);
    }
    free(line);
}

void getline_wrapper(struct roll_encoding *d, struct arguments *args) {
    size_t bufsize = 0;
    char *line;
    getline(&line, &bufsize, args->ist);
    if(line == NULL || line == 0 || feof(args->ist)) {
        d->quit = true;
        free(line);
        return;
    }
    if(0 == parse(d, line, bufsize)) {
        roll(d);
    }
    free(line);
}

void dice_init(struct roll_encoding *d) {
    d->ndice = 0;
    d->nsides = 0;
    d->shift = 0;
    d->quit = false;
}

int main(int argc, char** argv) {
    rl_bind_key('\t', rl_insert); // File completion is not relevant for this program

    struct arguments args;
    args.prompt = "\001\e[0;32m\002dice> \001\e[0m\002";
    if(isatty(fileno(stdin))) {
        args.mode = INTERACTIVE;
    } else {
        args.mode = PIPE;
    }
    args.seed = 0;
    args.ist = stdin;
    argp_parse(&argp, argc, argv, 0, 0, &args);

    struct roll_encoding *d = malloc(sizeof(struct roll_encoding));
    if(!d) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    dice_init(d);

    void (*process_next_line)(struct roll_encoding*, struct arguments*);
    switch(args.mode) {
        case INTERACTIVE:
            {
                process_next_line = &readline_wrapper;
            }
            break;
        case PIPE: case SCRIPTED:
            {
                process_next_line = &getline_wrapper;
            }
            break;
    }

    do {
        process_next_line(d, &args);
    } while(!(d->quit || feof(args.ist)));
    free(d);
    d = NULL;
    return 0;
}
