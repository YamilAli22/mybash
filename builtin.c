#include "builtin.h"
#include "command.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tests/syscall_mock.h"

const char *bt_cmds[3] = {
    "cd",
    "help",
    "exit"
};

bool builtin_is_internal(scommand cmd) {
    assert(cmd!=NULL);
    for (unsigned int i=0; i<3; ++i) {
        if (strcmp(scommand_front(cmd), bt_cmds[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool builtin_alone(pipeline p) {
    assert(p!=NULL);
    return pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p));
}

void builtin_run(scommand cmd) {
    assert(builtin_is_internal(cmd));
    if (strcmp(scommand_front(cmd), bt_cmds[0]) == 0) { // 'cd'
        scommand_pop_front(cmd); // saco 'cd' y me fijo si quedo algo o no para decidir a que directorio ir
        if (scommand_is_empty(cmd)) { // solo estaba 'cd' -> voy al directorio home
            char *home = getenv("HOME");
            if (chdir(home) == -1) {
                perror("chdir");
            }
        } else {
            // printf("builtin_run llamado con: %s\n", scommand_front(cmd));
            if (chdir(scommand_front(cmd)) == -1) { // voy al directorio que indica lo que devuelve scommand_front
                perror("chdir");
            }
        }
    
    } else if (strcmp(scommand_front(cmd), bt_cmds[1]) == 0) { // 'help'
        printf("Welcome to MyBash!! This mini-shell has been developed by Yamil Ali\n"
                "The builtin commands that you can use are these: \n"
               " - 'cd <path>' changes the directory to the specified path, if not path is specified, changes to the home directory.\n"
               " - 'help' shows you a description of each command (that you are reading right now).\n"
               " - 'exit' quit MyBash.\n");
    
    } else { // 'exit'
        exit(EXIT_SUCCESS);
    }
}
