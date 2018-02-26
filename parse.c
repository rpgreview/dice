#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <termcap.h> // Needed for clear_screen
#include <errno.h>
#include "parse.h"

static const struct cmd_map commands[] = {
    { quit, { "quit" } },
    { clear, { "clear" } },
};
#define NUMBER_OF_DEFINED_COMMANDS 2
#define CMD_MAX_STR_LEN 5

void clear_screen() {
    char buf[1024];
    tgetent(buf, getenv("TERM"));

    char *str;
    str = tgetstr("cl", NULL);
    fputs(str, stdout);
}

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
        } else if(*(buf + charnum) == '#') { //comment detected
            break;
        } else {
            switch(*(buf + charnum)) {
                case 'd': case 'D': case '+': case '-': case 'x':
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
                            if(charnum - offset >= LONG_MAX_STR_LEN && isdigit(*(buf + charnum))) {
                                printf("Invalid numeric input detected. The maximum number allowed is %ld (LONG_MAX).\n", LONG_MAX);
                                free(tok_str);
                                return 1;
                            }
                            tok_str[charnum - offset] = '\0';
                            errno = 0;
                            long num = strtol(tok_str, NULL, 10);
                            if((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))
                                || (errno != 0 && num == 0)) {
                                printf("Error %d (%s) converting string '%s' to number.\n", errno, strerror(errno), tok_str);
                                return 1;
                            }
                            t[*tokens_found].type = number;
                            t[*tokens_found].number = num;
                        } else if(isalpha(*(buf + charnum))) {
                            int numchars = 0;
                            while(isalpha(*(buf + charnum))) {
                                ++numchars;
                                ++charnum;
                            }
                            tok_str = malloc((numchars + 1)*sizeof(char));
                            if(!tok_str) {
                                fprintf(stderr, "Error allocating memory.\n");
                                exit(1);
                            }
                            memset(tok_str, 0, numchars + 1);
                            charnum = offset;
                            while(isalpha(*(buf + charnum)) && charnum - offset < numchars) {
                                tok_str[charnum - offset] = *(buf + charnum);
                                ++charnum;
                            }
                            tok_str[charnum - offset] = '\0';
                            t[*tokens_found].type = command;
                            int cmd_num = 0;
                            bool cmd_found = false;
                            int max_str_len = CMD_MAX_STR_LEN >= numchars ? CMD_MAX_STR_LEN : numchars;
                            while(cmd_num < NUMBER_OF_DEFINED_COMMANDS) {
                                if(0 == strncmp(tok_str, commands[cmd_num].cmd_str, max_str_len)) {
                                    t[*tokens_found].cmd = commands[cmd_num].cmd_code;
                                    cmd_found = true;
                                    break;
                                }
                                ++cmd_num;
                            }
                            if(!cmd_found) {
                                printf("Unknown command: %s\n", tok_str);
                                return 1;
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

void print_state_name(const state_t s) {
    switch(s) {
        case error:
            printf("error");
            break;
        case start:
            printf("start");
            break;
        case operator_pending:
            printf("operator_pending");
            break;
        case recv_x:
            printf("recv_x");
            break;
        case recv_num_dice:
            printf("recv_num_dice");
            break;
        case recv_d:
            printf("recv_d");
            break;
        case recv_num_sides:
            printf("recv_num_sides");
            break;
        case recv_shift_dir:
            printf("recv_shift_dir");
            break;
        case recv_shift_amount:
            printf("recv_shift_amount");
            break;
        case recv_cmd:
            printf("recv_cmd");
            break;
        case finish:
            printf("finish");
            break;
        default:
            printf("undefined");
    }
}

/*
    Valid state transitions:
        start               -> operator_pending, recv_d, recv_cmd, finish
        operator_pending    -> recv_x, recv_d, error
        recv_x              -> recv_num_dice, recv_d, error
        recv_num_dice       -> recv_d, error
        recv_d              -> recv_num_sides, error
        recv_num_sides      -> recv_shift_dir, finish, error
        recv_shift_dir      -> recv_shift_amount, error
        recv_shift_amount   -> finish
        recv_cmd            -> finish, error
*/

int parse(struct roll_encoding *restrict d, const char *buf, const size_t len) {
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
    int tmp = 0;
    for(toknum = 0; toknum < tokens_found && !(s == finish || s == error); ++toknum) {
        switch(t[toknum].type) {
            case none:
                s = error;
                printf("Nothing to do.\n");
                break;
            case number:
                switch(s) {
                    case start:
                        s = operator_pending;
                        tmp = t[toknum].number;
                        break;
                    case recv_x:
                        s = recv_num_dice;
                        d->ndice = t[toknum].number;
                        break;
                    case recv_d:
                        if(t[toknum].number > RAND_MAX) {
                            s = error;
                            printf("The maximum number of sides a dice can have is %d (RAND_MAX).\n", RAND_MAX);
                        } else {
                            s = recv_num_sides;
                            d->nsides = t[toknum].number;
                        }
                        break;
                    case recv_shift_dir:
                        s = recv_shift_amount;
                        d->shift *= t[toknum].number;
                        break;
                    default:
                        printf("Cannot process number '%ld' while in state '", t[toknum].number);
                        print_state_name(s);
                        printf("'\n");
                        s = error;
                }
                break;
            case operator:
                switch(s) {
                    case start:
                        switch(t[toknum].op) {
                            case 'd': case 'D':
                                d->ndice = 1;
                                s = recv_d;
                                break;
                            default:
                                printf("Cannot process operator '%c' while in state '", t[toknum].op);
                                print_state_name(s);
                                printf("'\n");
                                s = error;
                        }
                        break;
                    case operator_pending:
                        switch(t[toknum].op) {
                            case 'd': case 'D':
                                d->ndice = tmp;
                                s = recv_d;
                                break;
                            case 'x':
                                d->nreps = tmp;
                                s = recv_x;
                                break;
                            default:
                                printf("Cannot process operator '%c' while in state '", t[toknum].op);
                                print_state_name(s);
                                printf("'\n");
                                s = error;
                        }
                        break;
                    case recv_x:
                        switch(t[toknum].op) {
                            case 'd': case 'D':
                                d->ndice = 1;
                                s = recv_d;
                                break;
                            default:
                                printf("Cannot process operator '%c' while in state '", t[toknum].op);
                                print_state_name(s);
                                printf("'\n");
                                s = error;
                        }
                        break;
                    case recv_num_dice:
                        switch(t[toknum].op) {
                            case 'd': case 'D':
                                s = recv_d;
                                break;
                            default:
                                printf("Cannot process operator '%c' while in state '", t[toknum].op);
                                print_state_name(s);
                                printf("'\n");
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
                                printf("Cannot process operator '%c' while in state '", t[toknum].op);
                                print_state_name(s);
                                printf("'\n");
                                s = error;
                        }
                        break;
                    default:
                        printf("Cannot process operator '%c' while in state '", t[toknum].op);
                        print_state_name(s);
                        printf("'\n");
                        s = error;
                }
                break;
            case command:
                switch(s) {
                    case start:
                        s = recv_cmd;
                        switch(t[toknum].cmd) {
                            case quit:
                                d->suppress = true;
                                d->quit = true;
                                break;
                            case clear:
                                d->suppress = true;
                                clear_screen();
                                break;
                            default:
                                printf("Received invalid command.\n");
                                s = error;
                        }
                        break;
                    default:
                        printf("Commands may not follow other expressions.\n");
                        s = error;
                }
                break;
            case end:
                switch(s) {
                    case start:
                        s = finish;
                        return 1;
                        break;
                    case recv_num_sides: case recv_shift_amount: case recv_cmd:
                        s = finish;
                        break;
                    default:
                        printf("Cannot process end token while in state '");
                        print_state_name(s);
                        printf("'\n");
                        s = error;
                }
                break;
            default:
                printf("Unknown token type '%d' detected while in state '", t[toknum].type);
                print_state_name(s);
                printf("'\n");
                s = error;
        }
    }
    if(s == finish && !d->suppress) {
        return 0;
    } else {
        if(s == error) {
            dice_init(d);
        }
        return 1;
    }
}
