#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <termcap.h> // Needed for clear_screen
#include <errno.h>
#include <omp.h>
#include "parse.h"
#include "roll-engine.h"

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

void token_list_init(struct token *t, const size_t len) {
    int toknum;
    #pragma omp for schedule(static) private(toknum) nowait
    for(toknum = 0; toknum < len; ++toknum) {
        t[toknum].type = none;
        t[toknum].number = 0;
        t[toknum].op = '?';
        t[toknum].cmd = -1;
    }
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
                case 'd': case 'D':
                    {
                        t[*tokens_found].type = dice_operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                case '+': case '-':
                    {
                        t[*tokens_found].type = additive_operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                case 'x': case 'X':
                    {
                        t[*tokens_found].type = rep_operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                case '!':
                    {
                        t[*tokens_found].type = explode_operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                case 'T':
                    {
                        t[*tokens_found].type = threshold_operator;
                        t[*tokens_found].op = *(buf + charnum);
                        ++charnum;
                    }
                    break;
                case ';':
                    {
                        t[*tokens_found].type = statement_delimiter;
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
    t[*tokens_found].type = eol;
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
        case decide_reps_or_rolls:
            printf("decide_reps_or_rolls");
            break;
        case want_number_of_sides:
            printf("want_number_of_sides");
            break;
        case check_number_of_dice:
            printf("check_number_of_dice");
            break;
        case want_roll:
            printf("want_roll");
            break;
        case check_dice_operator:
            printf("check_dice_operator");
            break;
        case check_explode_or_more_rolls:
            printf("check_explode_or_more_rolls");
            break;
        case check_more_rolls:
            printf("check_more_rolls");
            break;
        case check_end:
            printf("check_end");
            break;
        case finish:
            printf("finish");
            break;
        default:
            printf("undefined");
    }
}

void print_token_type(const token_t type) {
    switch(type) {
        case none:
            printf("none");
            break;
        case number:
            printf("number");
            break;
        case dice_operator:
            printf("dice_operator");
            break;
        case rep_operator:
            printf("rep_operator");
            break;
        case additive_operator:
            printf("additive_operator");
            break;
        case explode_operator:
            printf("explode_operator");
            break;
        case command:
            printf("command");
            break;
        case statement_delimiter:
            printf("statement_delimiter");
            break;
        case eol:
            printf("eol");
            break;
        default:
            printf("unknown");
    }
}

void print_token_payload(const struct token *tok) {
    switch(tok->type) {
        case number:
            printf("%ld", tok->number);
            break;
        case dice_operator: case rep_operator: case additive_operator: case explode_operator: case statement_delimiter:
            printf("%c", tok->op);
            break;
        case command:
            printf("%s", commands[tok->cmd].cmd_str);
            break;
        default:
            printf("N/A");
    }
}

void print_token_info(const struct token *tok) {
    printf("{type: ");
    print_token_type(tok->type);
    printf(", payload: ");
    print_token_payload(tok);
    printf("}");
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

void process_none(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    printf("Nothing to do.\n");
    *s = error;
}

void process_number(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case start:
            *s = decide_reps_or_rolls;
            *tmp = tok->number;
            break;
        case want_roll:
            t->last_roll->ndice = tok->number;
            t->ndice += tok->number;
            *s = check_dice_operator;
            break;
        case want_number_of_sides:
            if(tok->number > RAND_MAX) {
                *s = error;
                printf("The maximum number of sides a dice can have is %d (RAND_MAX).\n", RAND_MAX);
            } else {
                *s = check_explode_or_more_rolls;
                t->last_roll->nsides = tok->number;
            }
            break;
        case check_number_of_dice:
            if(tok->number > LONG_MAX) {
                *s = error;
                printf("The maximum number of dice is %ld (LONG_MAX).\n", LONG_MAX);
            } else {
                t->last_roll->ndice = tok->number;
                *s = check_dice_operator;
            }
            break;
        case want_threshold:
            if(tok->number > LONG_MAX) {
                *s = error;
                printf("The maximum threshold is %ld (LONG_MAX).\n", LONG_MAX);
            } else {
                t->threshold = tok->number;
                *s = check_end;
            }
            break;
        default:
            printf("Cannot process number '%ld' while in state '", tok->number);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_dice_operator(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case start:
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->ndice = 1;
            t->ndice += 1;
            *s = want_number_of_sides;
            break;
        case want_roll:
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->ndice = 1;
            t->ndice += 1;
            *s = want_number_of_sides;
            break;
        case decide_reps_or_rolls:
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->ndice = *tmp;
            t->ndice += *tmp;
            *s = want_number_of_sides;
            break;
        case check_dice_operator:
            *s = want_number_of_sides;
            break;
        case check_number_of_dice:
            t->last_roll->ndice = 1;
            *s = want_number_of_sides;
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_rep_operator(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case decide_reps_or_rolls:
            t->nreps = *tmp;
            *s = want_roll;
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_additive_operator(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case start:
            *s = check_number_of_dice;
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->dir = tok->op == '+' ? pos : neg;
            break;
        case check_explode_or_more_rolls: case check_more_rolls:
            *s = check_number_of_dice;
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->dir = tok->op == '+' ? pos : neg;
            break;
        case decide_reps_or_rolls: // Deal with cases like 1+2d4. Need to process first number then set things up for the following.
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->nsides = 1;
            t->last_roll->ndice = *tmp;
            t->ndice += *tmp;
            // First number done, now set up for whatever follows.
            *s = check_number_of_dice;
            t->last_roll->next = malloc(sizeof(struct roll_encoding));
            t->last_roll = t->last_roll->next;
            dice_init(t->last_roll);
            t->last_roll->dir = tok->op == '+' ? pos : neg;
            break;
        case check_dice_operator: // Deal with cases like 2d4+1+3d6. The middle "roll" needs its nsides set to 1. This only happens if memory has already been allocated for the middle.
            t->last_roll->nsides = 1;
            *s = check_number_of_dice;
            t->last_roll->next = malloc(sizeof(struct roll_encoding));
            t->last_roll = t->last_roll->next;
            dice_init(t->last_roll);
            t->last_roll->dir = tok->op == '+' ? pos : neg;
            t->last_roll->nsides = 1;
            break;
        case want_roll:
            t->last_roll->dir = tok->op == '+' ? pos : neg;
            *s = check_number_of_dice;
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_explode_operator(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case check_explode_or_more_rolls:
            *s = check_more_rolls;
            t->last_roll->explode = true;
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_threshold_operator(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case check_dice_operator: case check_explode_or_more_rolls: case check_more_rolls:
            *s = want_threshold;
            t->use_threshold = true;
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
}

void process_command(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case start:
            *s = check_end;
            switch(tok->cmd) {
                case quit:
                    *s = finish;
                    t->suppress = true;
                    t->quit = true;
                    break;
                case clear:
                    t->suppress = true;
                    clear_screen();
                    break;
                default:
                    printf("Received invalid command.\n");
                    *s = error;
            }
            break;
        default:
            printf("Commands may not follow other expressions.\n");
            *s = error;
    }
}

void process_statement_delimiter(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    bool continuing = false;
    switch(*s) {
        case start:
            continuing = true;
            break;
        case decide_reps_or_rolls:
            continuing = true;
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->nsides = 1;
            t->last_roll->ndice = *tmp;
            t->ndice += *tmp;
            break;
        case check_dice_operator:
            continuing = true;
            t->last_roll->nsides = 1;
            break;
        case check_explode_or_more_rolls: case check_more_rolls:
            continuing = true;
            break;
        case check_end:
            continuing = true;
            break;
        default:
            printf("Cannot process operator '%c' while in state '", tok->op);
            print_state_name(*s);
            printf("'\n");
            *s = error;
    }
    if(continuing) {
        if(!t->suppress) {
            roll(t);
        }
        *s = start;
        parse_tree_reset(t);
    }
}

void process_eol(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    switch(*s) {
        case decide_reps_or_rolls:
            if(t->last_roll == NULL) {
                t->dice_specs = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->dice_specs;
            } else {
                t->last_roll->next = malloc(sizeof(struct roll_encoding));
                t->last_roll = t->last_roll->next;
            }
            dice_init(t->last_roll);
            t->last_roll->nsides = 1;
            t->last_roll->ndice = *tmp;
            t->ndice += *tmp;
            break;
        case check_dice_operator:
            t->last_roll->nsides = 1;
            break;
        default: {}
    }
    if(!t->suppress) {
        roll(t);
    }
    *s = finish;
    parse_tree_reset(t);
}

void process_default(struct token *tok, struct parse_tree *t, state_t *s, long* tmp) {
    printf("Unknown token type '%d' detected while in state '", tok->type);
    print_state_name(*s);
    printf("'\n");
    *s = error;
}

void parse_tree_reset(struct parse_tree *t) {
    t->suppress = false;
    t->quit = false;
    t->nreps = 1;
    t->ndice = 0;
    t->use_threshold = false;
    t->threshold = 0;
    t->last_roll = NULL;
    if(t->dice_specs != NULL) {
        dice_reset(t->dice_specs);
        t->dice_specs = NULL;
    }
}

int parse(struct parse_tree *t, const char *buf, const size_t len) {
    int tokens_found = 0;
    struct token toks[len];
    int lex_err = lex(toks, &tokens_found, buf, len);
    if(lex_err != 0) {
        return lex_err;
    }

    state_t s = start;
    parse_tree_reset(t);
    long tmp = 0;
    int toknum;
    void (*process_token)(struct token *tok, struct parse_tree *t, state_t *s, long* tmp);
    for(toknum = 0; toknum < tokens_found && !(s == finish || s == error); ++toknum) {
        switch(toks[toknum].type) {
            case none:
                process_token = process_none;
                break;
            case number:
                process_token = process_number;
                break;
            case dice_operator:
                process_token = process_dice_operator;
                break;
            case rep_operator:
                process_token = process_rep_operator;
                break;
            case additive_operator:
                process_token = process_additive_operator;
                break;
            case explode_operator:
                process_token = process_explode_operator;
                break;
            case threshold_operator:
                process_token = process_threshold_operator;
                break;
            case command:
                process_token = process_command;
                break;
            case statement_delimiter:
                process_token = process_statement_delimiter;
                break;
            case eol:
                process_token = process_eol;
                break;
            default:
                process_token = process_default;
        }
        process_token(toks + toknum, t, &s, &tmp);
        if(t->quit) {
            break;
        }
    }
    if(s == finish) {
        return 0;
    } else {
        if(s == error) {
            parse_tree_reset(t);
        }
        return 1;
    }
}
