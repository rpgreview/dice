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

struct roll_encoding {
    long ndice;
    long nsides;
    long shift;
    long seed;
    bool quit;
};

const char *argp_program_version = "Dice 0.1";
const char *argp_program_bug_address = "cryptarch@github";

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

int main(int argc, char** argv) {
    struct arguments args;
    args.prompt = '>';
    args.mode = INTERACTIVE;
    args.seed = "0";
    args.seed_set = false;
    argp_parse(&argp, argc, argv, 0, 0, &args);

    struct roll_encoding *d = (struct roll_encoding*)malloc(sizeof(struct roll_encoding));
    if(!d) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }

    // Set seed.
    char *s_endptr;
    d->seed = strtol(args.seed, &s_endptr, 10);
    if(!args.seed_set) {
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        d->seed = t.tv_nsec * t.tv_sec;
    }
    srandom(d->seed);

    if(args.mode == INTERACTIVE) {
        do {
            char prompt[3];
            memset(prompt, 0, 3);
            prompt[0] = args.prompt;
            prompt[1] = ' ';
            prompt[2] = '\0';
            char *line = readline(prompt);
            if(line == NULL || line == 0) {
                break;
            }
            size_t bufsize = strlen(line);
            if(0 == parse(d, line, bufsize)) {
                roll(d);
            }
            free(line);
        } while(!d->quit);
    }
    free(d);
    d = NULL;
    return 0;
}
