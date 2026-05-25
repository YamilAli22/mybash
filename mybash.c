#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"

char buffer[1024];

static void show_prompt(void) {
    char *cwd = getcwd(buffer, sizeof(buffer));
    if (cwd!=NULL) {
        printf("mybash>%s ", cwd);
    } else {
        printf ("mybash> ");
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

