#include "execute.h"
#include "command.h"
#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include "tests/syscall_mock.h"

/*
Las siguientes dos funciones son para manejar los procesos zombies. Los procesos zombies ocurren cuando un proceso termino pero nunca se recolectaron sus exit code, 
por lo que el proceso sigue en la process table a pesar de haber terminado. Al llamar a wait() en los procesos que corren en foreground,
el padre espera a que el hijo termine y su exit code es recolectado, limpiando al proceso hijo de la process table, esto no ocurre cuando uno (o más) procesos se están
ejecutando en background, por eso registramos un manejador de señal SIGCHLD que se ejecuta automáticamente cuando un hijo termina. 
Este manejador llama a waitpid con WNOHANG en un loop para recolectar todos los hijos que hayan terminado sin bloquear al padre, limpiándolos de la process table.
*/ 

static void sigchld_handler(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void init_signal_handlers(void) {
    signal(SIGCHLD, sigchld_handler);
}

// TODO: CHEQUEAR ERRORES EN LAS SYSCALLS Y MANEJARLOS

void execute_command(scommand s_cmd) {
    assert(s_cmd!=NULL);
    // caso donde el pipe tiene un solo comando y es interno
    // char *string = scommand_to_string(s_cmd);
    // printf("comando: %s\n", string);
    bool b_alone = builtin_is_internal(s_cmd);     
    if (b_alone) {
        builtin_run(s_cmd);
        return;
     }
    unsigned int cmd_length = scommand_length(s_cmd);
    char **argv = malloc((cmd_length + 1) * sizeof(char *));
    char *filename_in = scommand_get_redir_in(s_cmd);
    char *filename_out = scommand_get_redir_out(s_cmd);

    for (unsigned int i = 0; i < cmd_length; ++i) {
        unsigned int length_cmd = strlen(scommand_front(s_cmd)) + 1;
        argv[i] = malloc(length_cmd * sizeof(char));
        strcpy(argv[i], scommand_front(s_cmd));
        scommand_pop_front(s_cmd);
    }
    argv[cmd_length] = NULL;

    // Todas estas syscalls deberian ser chequeadas en su valor de retorno
    if (filename_in != NULL) {
        int fd = open(filename_in, O_RDONLY, 0);
        if (fd == -1) {
            perror(filename_in);
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO); // STDIN ahora apunta al archivo y dup2 hace el close internamente
        close(fd); // cierro el fd que cree con open porque ya no lo necesito
    }
    if (filename_out != NULL) {
        int fd = open(filename_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror(filename_out);
            exit(EXIT_FAILURE);
        }
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

    for (unsigned int i=0; i<num_cmds; ++i) {
        if (i!=0) {
            // para todos los comandos excepto el primero, seteo read_fd y write_fd para que apunten al pipe creado en el paso anterior (antes de crear un nuevo pipe para el paso actual)
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

        } else if (rc==0) { // creo que todo esto se podría simplificar usando popen y pclose, pero para entender el funcionamiento es mejor hacerlo así 
            if (i != num_cmds - 1) {
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            if (i!=0) {
                close(write_fd);
                dup2(read_fd, STDIN_FILENO);
                close(read_fd);
            }
            execute_command(pipeline_front(apipe));
        }
        else { // padre
            if (i!=0) {
                /* printf("padre iter %d: cierra read_fd=%d write_fd=%d\n", i, read_fd, write_fd); */
                close(read_fd);
                close(write_fd);
            }
            pipeline_pop_front(apipe);
        }
    }
    if (wt) {
        for (unsigned int i=0; i<num_cmds; ++i) {
            wait(NULL);
        }
    }
}

/*
Pasos básicos para implementar el piping que encontré en stack overflow:
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
