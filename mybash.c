#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"

char buff[1024];
char hostname[256];

static void show_prompt(void) {
    char *cwd = getcwd(buff, sizeof(buff));
    gethostname(hostname, sizeof(hostname));
    char *user = getenv("USER");
    if (cwd!=NULL) {
        fprintf(stdout, "%s@%s:%s$ ", user, hostname, cwd);
    } else {
        fprintf (stdout, "%s@%s:$ ", user, hostname);
    }
    fflush (stdout);
}

int main(int argc, char *argv[]) {
    init_signal_handlers();
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt();
        pipe = parse_pipeline(input);
        if (pipe!=NULL) {
            execute_pipeline(pipe);
            pipeline_destroy(pipe);
            pipe = NULL;
        }
        quit = parser_at_eof(input);
    }
    parser_destroy(input); input = NULL;
    return EXIT_SUCCESS;
}

