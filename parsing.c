#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {
    /* Devuelve NULL cuando hay un error de parseo */
    if (parser_at_eof(p)) {
        return NULL;
        return NULL;}
    scommand cmd = scommand_new();
    arg_kind_t type;
    char *arg;

    parser_skip_blanks(p); // no sé si hace falta esto
    while (!parser_at_eof(p)) {
        arg = parser_next_argument(p, &type);
        if (arg==NULL) {
            break;
            // o retorno el cmd? 
        }  
        if (type == ARG_NORMAL) {
            scommand_push_back(cmd, arg); 
        }
        if (type == ARG_INPUT) {
            scommand_set_redir_in(cmd, arg);
        }
        if (type == ARG_OUTPUT) {
            scommand_set_redir_out(cmd, arg);
        }
        parser_skip_blanks(p); // o esto 
    }
    
    if (scommand_is_empty(cmd)) {
        scommand_destroy(cmd);
        return NULL;
    }
    return cmd;
}

pipeline parse_pipeline(Parser p) {
    assert(p!=NULL); 
    pipeline result = pipeline_new();
    if (parser_at_eof(p)) {
        pipeline_destroy(result);
        return NULL;
    }
    scommand cmd = NULL;
    bool error = false, another_pipe=true;
    bool is_bg = false;
    bool garbage = false;

    cmd = parse_scommand(p);
    parser_skip_blanks(p);
    error = (cmd==NULL); /* Comando inválido al empezar */
    if (error) {
        printf("Comando inválido\n");
        pipeline_destroy(result);
        return NULL;
    }

    pipeline_push_back(result, cmd);
    while (another_pipe && !error) {
        parser_op_pipe(p, &another_pipe);
        if (another_pipe) {
            cmd = parse_scommand(p);
            parser_skip_blanks(p);
            if (cmd==NULL) {
                error = true;
            } else {
                pipeline_push_back(result, cmd);
            }
        }
    }

    if (error) {
        parser_garbage(p, &garbage);
        printf("Error procesando el pipeline\n");
        pipeline_destroy(result);
        return NULL;
    }

    /* Opcionalmente un OP_BACKGROUND al final */
    parser_op_background(p, &is_bg);
    pipeline_set_wait(result, !is_bg);

    parser_garbage(p, &garbage);
    if (garbage) {
        char *gbg_message = parser_last_garbage(p);
        printf("Comando inválido: %s\n", gbg_message);
        pipeline_destroy(result);
        return NULL;
    }     
    return result;
}

