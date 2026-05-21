#include "execute.h"
#include "command.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>

void execute_command(scommand s_cmd) {
    assert(s_cmd!=NULL);
    // caso donde el pipe tiene un solo comando y es interno
    bool b_alone = builtin_is_internal(s_cmd);     
    if (b_alone) {
        builtin_run(s_cmd);
        return;
    }
    char **argv = malloc((scommand_length(s_cmd) + 1) * sizeof(char *));
    char *filename_in = scommand_get_redir_in(s_cmd);
    char *filename_out = scommand_get_redir_out(s_cmd);

    for (int i = 0; i < scommand_length(s_cmd); ++i) {
        argv[i] = scommand_front(s_cmd);
        scommand_pop_front(s_cmd);
    }
    argv[scommand_length(s_cmd)] = NULL;

    // Todas estas syscalls deberian ser chequeadas en su valor de retorno
    if (filename_in != NULL) {
        int fd = open(filename_in, O_RDONLY);
        dup2(fd, STDIN_FILENO); // STDIN ahora apunta al archivo y dup2 hace el close internamente
        close(fd); // cierro el fd que cree con open porque ya no lo necesito
    }
    if (filename_out != NULL) {
        int fd = open(filename_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    if (execvp(argv[0], argv) == -1) {
        free(argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    }    
}

// caso de 2 o mas comandos en el pipeline, acá tenemos que ver si son internos o no
void execute_pipeline(pipeline apipe) {
    assert(apipe!=NULL);
    if (builtin_alone(apipe)) {
        builtin_run(pipeline_front(apipe));
        return;
    }
    unsigned int num_cmds = pipeline_length(apipe);
    bool wt = pipeline_get_wait(apipe);
    int fd[2];
    int read_fd, write_fd = -1;

    for (int i=0; i<num_cmds; ++i) {
        if (i!=0) {
            read_fd = fd[0];
            write_fd = fd[1];
        }

        if (i != num_cmds - 1) { // si no estoy en el ultimo comando, creo el pipe
            pipe(fd);
        }

        pid_t rc = fork();
        if (rc < 0) {
            perror("fork\n");
            return;

        } else if (rc==0) {    
        if (i!=0) {
            dup2(read_fd, STDIN_FILENO);
            close(read_fd);
            close(write_fd);
            }
        if (i != num_cmds - 1) {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            }
        execute_command(pipeline_front(apipe));
        }
        else { // padre
            if (i!=0) {
                close(read_fd);
                close(write_fd);
            }
            pipeline_pop_front(apipe);
        }
    }
    if (wt) {
        wait(NULL);
    }
}  

/* Pasos básicos para implementar el piping que encontré en stack overflow:
1. Create a pipe.
2. Execute fork().
3. In the child: overwrite standard input with the read end of the previous pipe, and standard output with the write end of the current pipe.
4. In the child: execute execvp().
5. In the parent: close unneeded pipes and save read end of current pipe to be used in the next iteration.

Básicamente la idea es crear un pipe para cada comando, excepto el último, e ir redirigiendo el stdin y el stdout a las puntas de los pipes 
y mantener la referencia de las puntas del pipe del comando en que estoy actualmente, para que el comando siguiente 
pueda leer desde la punta del pipe anterior. Osea, pipe del comando i (i>0) lee desde el read end del pipe del comando i-1, escribe lo que tenga que 
escribir, y "guarda" su read en para que el comando i+1 haga lo mismo que él. Cerras las puntas que ya no se usan.

*/
