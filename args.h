#ifndef __ARGS_H__
#define __ARGS_H__
#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <string.h>
#include "io.h"

const char *argp_program_version = "Dice 0.8";
const char *argp_program_bug_address = "cryptarch@github";

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] = {
    {"prompt",  'p', "STRING", 0, "Set the dice interactive prompt to STRING.\n(Default: 'dice> ')"},
    {"seed", 's', "NUMBER", 0, "Set the seed to NUMBER. (Default is based on current time.)"},
    {"help", 'h', NULL, 0, "Print this help message."},
    {"version", 'v', NULL, 0, "Print version information."},
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
                char *arg_iter;
                for(arg_iter = arg; *arg_iter != '\0'; ++arg_iter) {
                    if(!isdigit(*arg_iter)) {
                        fprintf(stderr, "The random seed must be a number between 0 and %u.\n", UINT_MAX);
                        exit(1);
                    }
                }
                char *s_endptr;
                errno = 0;
                arguments->seed = strtol(arg, &s_endptr, 10);
                if((errno == ERANGE && (arguments->seed == LONG_MAX || arguments->seed == LONG_MIN))
                    || (errno != 0 && arguments->seed == 0)) {
                    fprintf(stderr, "Error %d (%s) setting seed to %s.\n", errno, strerror(errno), arg);
                    arguments->seed_set = false;
                    return 1;
                } else {
                    arguments->seed_set = true;
                }
            }
            break;
        case 'v':
            {
                printf("%s\n", argp_program_version);
                exit(0);
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
                            fprintf(stderr, "Error %d (%s) opening file %s\n", errno, strerror(errno), arg);
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
