#ifndef AST_WORKER
#define AST_WORKER


struct node_t *normalize_tree(struct node_t *node);
struct node_t *optimize_tree(struct node_t *node);

extern const size_t optimization_rules_len;
extern struct node_t *(*optimization_rules[])(struct node_t *node, double time);


#endif
