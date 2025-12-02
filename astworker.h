#ifndef AST_WORKER
#define AST_WORKER

#include "parser_api.h"

int is_same(struct node_t *a, struct node_t *b);
struct node_t *normalize_tree(struct node_t *node);
struct node_t *optimize_tree(struct node_t *node, int N, int K);

extern const size_t optimization_rules_len;
extern struct node_t *(*optimization_rules[])(const struct node_t *node, double time);


#endif
