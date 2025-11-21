#ifndef EXPORT_H
#define EXPORT_H


#include "parser_api.h"
#include "parser_errors.h"


char *export_basic(char *s, struct node_t *node);
char *export_ast(char *s, struct node_t *node);


#endif
