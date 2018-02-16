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

void dice_init(struct roll_encoding *d) {
    d->ndice = 0;
    d->nsides = 0;
    d->shift = 0;
    d->quit = false;
}

typedef enum token_t {
    none = 0,
    number,
    operator,
    command,
    end
} token_t;

typedef enum cmd_t {
    quit
} cmd_t;

struct cmd_map {
    cmd_t cmd_code;
    char cmd_str[8];
};

struct cmd_map commands[] = {
    { quit, { "quit" } }
};
#define NUMBER_OF_DEFINED_COMMANDS 1
#define CMD_MAX_STR_LEN 4

struct token {
    token_t type;
    long number;
    char op;
    cmd_t cmd;
};

void token_init(struct token *t) {
    t->type = none;
    t->number = 0;
    t->op = '?';
    t->cmd = -1;
}

int lex(struct token *t, int *tokens_found, const char *buf, const size_t len) {
    int charnum = 0;
    *tokens_found = 0;
    while(*(buf + charnum) != '\0' && charnum <= len) {
        if(isspace(*(buf + charnum))) {
            ++charnum;
        } else {
            switch(*(buf + charnum)) {
                case 'd': case 'D': case '+': case '-':
                    {
                        t[*tokens_found].type = operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                default:
                    {
                        char *tok_str;
                        int offset = charnum;
                        if(isdigit(*(buf + charnum))) {
                            tok_str = malloc((LONG_MAX_STR_LEN + 1)*sizeof(char));
                            if(!tok_str) {
                                fprintf(stderr, "Error allocating memory.\n");
                                exit(1);
                            }
                            memset(tok_str, 0, LONG_MAX_STR_LEN + 1);
                            while(isdigit(*(buf + charnum)) && charnum - offset < LONG_MAX_STR_LEN) {
                                tok_str[charnum - offset] = *(buf + charnum);
                                ++charnum;
                            }
                            tok_str[charnum - offset] = '\0';
                            errno = 0;
                            long num = strtol(tok_str, NULL, 10);
                            if((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))
                                || (errno != 0 && num == 0)) {
                                printf("Error converting string '%s' to number.\n", tok_str);
                                return 1;
                            }
                            t[*tokens_found].type = number;
                            t[*tokens_found].number = num;
                        } else if(isalpha(*(buf + charnum))) {
                            tok_str = malloc((CMD_MAX_STR_LEN + 1)*sizeof(char));
                            if(!tok_str) {
                                fprintf(stderr, "Error allocating memory.\n");
                                exit(1);
                            }
                            memset(tok_str, 0, CMD_MAX_STR_LEN + 1);
                            while(isalpha(*(buf + charnum)) && charnum - offset < CMD_MAX_STR_LEN) {
                                tok_str[charnum - offset] = *(buf + charnum);
                                ++charnum;
                            }
                            tok_str[charnum - offset] = '\0';
                            t[*tokens_found].type = command;
                            int cmd_num = 0;
                            while(cmd_num < NUMBER_OF_DEFINED_COMMANDS) {
                                if(0 == strncmp(tok_str, commands[cmd_num].cmd_str, CMD_MAX_STR_LEN)) {
                                    t[*tokens_found].cmd = commands[cmd_num].cmd_code;
                                }
                                ++cmd_num;
                            }
                        } else {
                            printf("Unknown token detected: %c\n", *(buf + charnum));
                            return 1;
                        }
                        if(tok_str) {
                            free(tok_str);
                            tok_str = NULL;
                        }
                    }
            }
            (*tokens_found)++;
        }
    }
    t[*tokens_found].type = end;
    ++(*tokens_found);
    return 0;
}

typedef enum state_t {
    error = -1,
    start = 0,
    recv_num_dice,      // Always number
    recv_d,             // Always operator 'd'/'D'
    recv_num_sides,     // Always followed by number
    recv_shift_dir,     // Always operator '+'/'-'
    recv_shift_amount,  // Always number
    recv_cmd,           // Always string
    finish
} state_t;

/*
    Valid state transitions:
        start               -> recv_num_dice, recv_d, recv_cmd, finish
        recv_num_dice       -> recv_d, error
        recv_d              -> recv_num_sides, error
        recv_num_sides      -> recv_shift_dir, finish, error
        recv_shift_dir      -> recv_shift_amount, error
        recv_shift_amount   -> finish
        recv_cmd            -> finish, error
*/

int parse(struct roll_encoding *d, const char *buf, const size_t len) {
    int tokens_found = 0;
    struct token t[len];
    int toknum;
    for(toknum = 0; toknum < len; ++toknum) {
        token_init(&(t[toknum]));
    }
    int lex_err = lex(t, &tokens_found, buf, len);
    if(lex_err != 0) {
        return lex_err;
    }

    state_t s = start;
    dice_init(d);
    for(toknum = 0; toknum < tokens_found && !(s == finish || s == error); ++toknum) {
        switch(t[toknum].type) {
            case none:
                s = error;
                break;
            case number:
                switch(s) {
                    case start:
                        s = recv_num_dice;
                        d->ndice = t[toknum].number;
                        break;
                    case recv_d:
                        s = recv_num_sides;
                        d->nsides = t[toknum].number;
                        break;
                    case recv_shift_dir:
                        s = recv_shift_amount;
                        d->shift *= t[toknum].number;
                        break;
                    default:
                        s = error;
                }
                break;
            case operator:
                switch(s) {
                    case start: case recv_num_dice:
                        switch(t[toknum].op) {
                            case 'd': case 'D':
                                if(s == start) {
                                    d->ndice = 1;
                                }
                                s = recv_d;
                                break;
                            default:
                                s = error;
                        }
                        break;
                    case recv_num_sides:
                        switch(t[toknum].op) {
                            case '+': case '-':
                                s = recv_shift_dir;
                                d->shift = t[toknum].op == '+' ? 1 : -1;
                                break;
                            default:
                                s = error;
                        }
                        break;
                    default:
                        s = error;
                }
                break;
            case command:
                switch(s) {
                    case start:
                        s = recv_cmd;
                        switch(t[toknum].cmd) {
                            case quit:
                                d->quit = true;
                                break;
                            default:
                                s = error;
                        }
                        break;
                    default:
                        s = error;
                }
                break;
            case end:
                switch(s) {
                    case recv_num_sides: case recv_shift_amount: case recv_cmd:
                        s = finish;
                        break;
                    default:
                        s = error;
                }
                break;
            default:
                s = error;
        }
    }
    if(s == finish) {
        return 0;
    } else {
        return 1;
    }
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
    int parse_success = parse(d, line, bufsize);
    if(0 == parse_success) {
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

int main(int argc, char** argv) {
    rl_bind_key('\t', rl_insert); // File completion is not relevant for this program

    struct arguments args;
    args.prompt = "\001\e[0;32m\002dice> \001\e[0m\002";
    if(isatty(fileno(stdin))) {
        args.mode = INTERACTIVE;
    } else {
        args.mode = PIPE;
    }
    args.ist = stdin;

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    args.seed = t.tv_nsec * t.tv_sec;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    srandom(args.seed);

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
