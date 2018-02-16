#ifndef __ARGS_H__
#define __ARGS_H__
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <argp.h>
#include <unistd.h>

typedef enum invocation_type {
    INTERACTIVE = 0,
    SCRIPTED,
    PIPE
} invocation_type;

/* This structure is used by main to communicate with parse_opt. */
struct arguments {
    char *prompt;
    invocation_type mode;
    unsigned int seed;
    FILE *ist;
};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] = {
    {"prompt",  'p', "STRING", 0, "Set the dice interactive prompt to STRING.\n(Default: 'dice> ')"},
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
                int prompt_length = strlen(arg);
                arguments->prompt = (char*)malloc(prompt_length*sizeof(char));
                memset(arguments->prompt, 0, prompt_length);
                strncpy(arguments->prompt, arg, prompt_length + 1);
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
                char *s_endptr;
                errno = 0;
                arguments->seed = strtol(arg, &s_endptr, 10);
                if((errno == ERANGE && (arguments->seed == LONG_MAX || arguments->seed == LONG_MIN))
                    || (errno != 0 && arguments->seed == 0)) {
                    fprintf(stderr, "Error %d setting seed.\n", errno);
                    return 1;
                }
            }
            break;
        case ARGP_KEY_ARG:
            {
                {
                    arguments->mode = SCRIPTED;
                    if(0 != strcmp(arg, "-")) { 
                        errno = 0;
                        arguments->ist = fopen(arg, "r");
                        if(arguments->ist == NULL) {
                            fprintf(stderr, "Error %d opening file %s\n", errno, arg);
                            exit(1);
                        }
                    }
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
static char args_doc[] = "[file]";

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
