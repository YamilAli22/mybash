#include <sched.h>
#include <stdbool.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "command.h"
#include "strextra.h"

typedef struct scommand_s {
		GQueue *cmd;
		char *redin;
		char *redout;
} *scommand;

scommand scommand_new(void) {
	scommand new_cmd = malloc(sizeof(*new_cmd));
	if (new_cmd == NULL) {
			return NULL;
	}
	new_cmd->cmd = g_queue_new();
	new_cmd->redin = NULL;
	new_cmd->redout = NULL;
	assert(new_cmd!=NULL && scommand_is_empty(new_cmd) && scommand_get_redir_in(new_cmd) == NULL && scommand_get_redir_out(new_cmd) == NULL);
	return new_cmd;
}

scommand scommand_destroy(scommand self) {
	assert(self!=NULL);
	g_queue_free_full(self->cmd, free);
	free(self->redin);
	free(self->redout);
	free(self);
    self = NULL;
	return NULL;
}

void scommand_push_back(scommand self, char *arg) {
	assert(self!=NULL && arg!=NULL);
	g_queue_push_tail(self->cmd, arg);
	assert(!scommand_is_empty(self));
}

void scommand_pop_front(scommand self) {
	assert(self!=NULL && !scommand_is_empty(self));
    g_queue_pop_head(self->cmd);
}

void scommand_set_redir_in(scommand self, char * filename) {
    assert(self!=NULL);
    if (filename == NULL) {
        self->redin = NULL;
    }
    self->redin = filename;
}

void scommand_set_redir_out(scommand self, char * filename) {
    assert(self!=NULL);
    if (filename == NULL) {
        self->redout = NULL;
    }
    self->redout = filename;
}

bool scommand_is_empty(const scommand self) {
    assert(self!=NULL);
    bool is_empty = g_queue_is_empty(self->cmd);
    return is_empty;
}

unsigned int scommand_length(const scommand self) {
    assert(self!=NULL);
    unsigned int length = g_queue_get_length(self->cmd);
    return length;
}

char * scommand_front(const scommand self) {
    assert(self!=NULL && !scommand_is_empty(self));
    char * front_str = g_queue_peek_head(self->cmd);
    assert(front_str!=NULL);
    return front_str;
}

char * scommand_get_redir_in(const scommand self) {
    assert(self!=NULL);
    char *redir_in = self->redin;
    return redir_in;
}

char * scommand_get_redir_out(const scommand self) {
    assert(self!=NULL);
    char *redir_out = self->redout;
    return redir_out;
}

char * scommand_to_string(const scommand self) {
    assert(self!=NULL);
    char *result = strdup("");
    char *temp = NULL;
    for (unsigned int i=0; i<scommand_length(self); i++) {
        if (i>0) {
            temp = result; // esto es para guardar la referencia vieja
            result = strmerge(result, " "); // aca result apunta a memoria nueva
            free(temp); // aca libero esa referencia vieja
        }
        temp = result;
        result = strmerge(result, g_queue_peek_nth(self->cmd, i));
        free(temp);
    }
    if (self->redin!=NULL) {
        temp = result;
        result = strmerge(result, " < ");
        free(temp);
        temp = result;
        result = strmerge(result, self->redin);
        free(temp);
    }
    if (self->redout!=NULL) {
        temp = result;
        result = strmerge(result, " > ");
        free(temp);
        temp = result;
        result = strmerge(result, self->redout);
        free(temp);
    }
    return result;
}

typedef struct pipeline_s {
    GQueue *pipe; // acá esta la lista de comandos simples
    bool wait;
} *pipeline;


pipeline pipeline_new(void) {
    pipeline new_pipe = malloc(sizeof(struct pipeline_s));
    if (new_pipe == NULL) {
        return NULL;
    }
    new_pipe->pipe = g_queue_new();
    new_pipe->wait = true;
    return new_pipe;
}

pipeline pipeline_destroy(pipeline self) {
    assert(self!=NULL);
    while (!pipeline_is_empty(self)) {
        pipeline_pop_front(self);
    }
    free(self);
    self = NULL;
    return self;
}

void pipeline_push_back(pipeline self, scommand sc) {
    assert(self!=NULL && sc != NULL);
    g_queue_push_tail(self->pipe, sc);
}

void pipeline_pop_front(pipeline self) {
    assert(self!=NULL && !pipeline_is_empty(self));
    scommand sc = (scommand)g_queue_pop_head(self->pipe);
    scommand_destroy(sc);
}

void pipeline_set_wait(pipeline self, const bool w) {
    assert(self!=NULL);
    self->wait = w;
}

bool pipeline_is_empty(const pipeline self) {
    assert(self!=NULL);
    return true ? g_queue_is_empty(self->pipe) : false;
}

unsigned int pipeline_length(const pipeline self) {
    assert(self!=NULL);
    unsigned int pipe_length = g_queue_get_length(self->pipe);
    return pipe_length;
}

scommand pipeline_front(const pipeline self) {
    assert(self!=NULL && !pipeline_is_empty(self));
    scommand front_sc = g_queue_peek_head(self->pipe);
    return front_sc;
}

bool pipeline_get_wait(const pipeline self) {
    assert(self!=NULL);
    return true ? self->wait == true : false;
}

char * pipeline_to_string(const pipeline self) {
    assert(self!=NULL);
    char *result = strdup("");
    char *temp = NULL;
    char *sc = NULL;
    for (unsigned int i=0; i<pipeline_length(self); i++) {
        if (i>0) {
            temp = result;
            result = strmerge(result, " | ");
            free(temp);
        }
        temp = result;
        sc = scommand_to_string(g_queue_peek_nth(self->pipe, i));
        result = strmerge(result, sc);
        free(temp);
    }
    if (!self->wait) {
        temp = result;
        result = strmerge(result, " &");
        free(temp);
    }
    return result;
}



