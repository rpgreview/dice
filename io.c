#define _GNU_SOURCE 1 // Needed for getline to avoid implicit function declaration, see `man 3 getline`
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h> // Needed to expand out history path eg involving '~'
#include <errno.h>

#include "io.h"
#include "parse.h"
#include "roll-engine.h"

void getline_wrapper(struct parse_tree *t, struct arguments *args) {
    size_t bufsize = 0;
    char *line = NULL;
    errno = 0;
    int getline_retval = getline(&line, &bufsize, args->ist);
    if(line == NULL || line == 0 || feof(args->ist) || errno != 0 || getline_retval < 0) {
        if(errno != 0) {
            printf("Error %d (%s) getting line for reading.\n", errno, strerror(errno));
        }
        t->quit = true;
        goto end_of_getline;
    }
    parse(t, line, bufsize);
    t->current = t;
    while(t->current != NULL) {
        if(!t->current->suppress) {
            size_t nresults = t->current->use_threshold ? 1 : t->current->nreps;
            long *results = malloc(nresults*sizeof(long));
            memset(results, 0, nresults*sizeof(long));
            roll(&results, t->current);
            for(long rep = 0; rep < nresults; ++rep) {
                printf("%s%ld", rep == 0 ? "" : " ", results[rep]);
            }
            printf("\n");
            free(results);
            results = NULL;
        }
        t->current = t->current->next;
    }
end_of_getline:
    free(line);
}

void no_read(struct parse_tree *t, struct arguments *args) {
    printf("Unknown mode. Not reading any lines.\n");
}

void readline_wrapper(struct parse_tree *t, struct arguments *args) {
    char *line = readline(args->prompt);
    if(line == NULL || line == 0) {
        printf("\n");
        t->quit = true;
        goto end_of_readline;
    }
    size_t bufsize = strlen(line);
    int parse_success = parse(t, line, bufsize);
    t->current = t;
    while(t->current != NULL) {
        if(!t->current->suppress) {
            size_t nresults = t->current->use_threshold ? 1 : t->current->nreps;
            long *results = malloc(nresults*sizeof(long));
            memset(results, 0, nresults*sizeof(long));
            roll(&results, t->current);
            for(long rep = 0; rep < nresults; ++rep) {
                printf("%s%ld", rep == 0 ? "" : " ", results[rep]);
            }
            printf("\n");
            free(results);
            results = NULL;
        }
        t->current = t->current->next;
    }
    if(0 == parse_success) {
        add_history(line);
    }
end_of_readline:
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
            case 2:
                // Ignore missing file error, it just means we're running interactively for the first time.
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
