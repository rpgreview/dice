#ifndef __ARGS_H__
#define __ARGS_H__
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>

typedef enum invocation_type {
    INTERACTIVE = 0,
    SCRIPTED = 1,
    COMMAND = 2
} invocation_type;

/* This structure is used by main to communicate with parse_opt. */
struct arguments {
    char prompt;
    invocation_type mode;
    char *cmd;
    int cmd_len;
    char *seed;
    bool seed_set;
};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] = {
    {"prompt",  'p', "CHARACTER", 0, "Set the dice interactive prompt to CHARACTER.\n(Default: '>')"},
    {0,  'c', 0, OPTION_HIDDEN, "Read commands from the first non-option argument. (Not yet implemented.)"},
    {"seed", 's', "NUMBER", 0, "Set the seed to NUMBER. (Default is based on current time.)"},
    {"help", 'h', NULL, 0, "Print this help message."},
    {0}
};

/*
   PARSER. Field 2 in ARGP.
   Order of parameters: KEY, ARG, STATE.
*/
static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 'p':
            {
                int prompt_length = strnlen(arg, 2);
                if(prompt_length == 1) {
                    arguments->prompt = *arg;
                } else {
                    fprintf(stderr, "The prompt string must be a single character.\n");
                    exit('p');
                }
            }
            break;
        case 'c':
            {
                arguments->mode = COMMAND;
            }
            break;
        case 'h':
            {
                argp_state_help(state, stdout, ARGP_HELP_USAGE | ARGP_HELP_LONG | ARGP_HELP_DOC);
                exit(EXIT_SUCCESS);
            }
            break;
        case 's':
            {
                arguments->seed = arg;
                arguments->seed_set = true;
            }
            break;
        case ARGP_KEY_ARG:
            {
                if(arguments->mode == COMMAND) {
                }
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments
     that we accept.
*/
static char args_doc[] = "[command | file]";

/*
  DOC.  Field 4 in ARGP.
  Program documentation.
*/
static char doc[] = "Dice -- An interpreter for standard dice notation (qv Wikipedia:Dice_notation)";

/*
   The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, args_doc, doc};
#endif // __ARGS_H__
