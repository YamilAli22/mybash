#include "command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#MAX_LENGTH 10

int main() {
		
     char *str = NULL;
    /* MAX_LENGTH veces el mismo comando simple */
    pipeline pipe = pipeline_new();
    for (int i=0; i<10; i++) {
        scommand cmd=scommand_new();
        scommand_push_back(cmd, strdup ("gtk-fuse"));
        pipeline_push_back (pipe, cmd);
    }
    pipeline_set_wait (pipe, false);
    str = pipeline_to_string (pipe);
    /* cuenta cuantos pipes hay en n */
    int i=0;
    int n=0;
    while (str[i]) {
        n += (str[i] == '|');
        i++;
    }
    printf("pipeline: %s\n", str);
    printf("cantidad de pipes: %d\n", n);    
    free(str);
    //scommand cmd = scommand_new();
    //scommand_destroy(cmd);
    //scommand_push_back(cmd, strdup("ls"));
    //scommand_push_back(cmd, strdup("-l"));
    //scommand_set_redir_in(cmd, "archivo.txt");
    //char *s = scommand_to_string(cmd);
    //printf("ESTO ES MAIN: %s\n", s);
	//printf("HOLA");	
    //free(s);
    //printf("HOLA2");
    //scommand_destroy(cmd);
    //printf("HOLA3");
    return 0;
}
