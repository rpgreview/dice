#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h> // Needed to expand out history path eg involving '~'
#include <errno.h>

#include "interactive.h"
#include "dice.h"
#include "parse.h"

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

void read_history_wrapper(const char *filename) {
    errno = 0;
    wordexp_t matched_paths;
    int we_ret = wordexp(filename, &matched_paths, 0);
    switch(we_ret) {
        case WRDE_NOSPACE:
            {
                fprintf(stderr, "Insufficient memory to expand path '%s'.\n", filename);
            }
            break;
        default:
            break;
    }
    char **path = matched_paths.we_wordv;
    int path_num = 0;
    for(path_num = 0; path_num < matched_paths.we_wordc; ++path_num) {
        errno = 0;
        read_history(path[path_num]);
        switch(errno) {
            case 0:
                break;
            default:
                fprintf(stderr, "Error %d (%s) opening %s for reading.\n", errno, strerror(errno), path[path_num]);
        }
    }
    wordfree(&matched_paths);
}

void write_history_wrapper(const char *filename) {
    errno = 0;
    wordexp_t matched_paths;
    int we_ret = wordexp(filename, &matched_paths, 0);
    switch(we_ret) {
        case WRDE_NOSPACE:
            {
                fprintf(stderr, "Insufficient memory to expand path '%s'.\n", filename);
            }
            break;
        default:
            break;
    }
    char **path = matched_paths.we_wordv;
    int path_num = 0;
    for(path_num = 0; path_num < matched_paths.we_wordc; ++path_num) {
        errno = 0;
        write_history(path[path_num]);
        switch(errno) {
            case 0:
                break;
            case 2:
                fprintf(stderr, "Created new history file '%s'.\n", path[path_num]);
                break;
            case 22:
                /*
                   It seems that when the history file already exists,
                   write_history emits an erroneous "Invalid argument"
                   despite successfully replacing history file contents.
                */
                break;
            default:
                fprintf(stderr, "Error %d (%s) opening %s for writing.\n", errno, strerror(errno), path[path_num]);
        }
    }
    wordfree(&matched_paths);
}
