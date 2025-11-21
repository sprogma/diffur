#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "math.h"
#include "string.h"
#include "ctype.h"
#include "stdio.h"


struct node_t *optimize_tree_inner(struct node_t *node, double time)
{
    /* 1. optimize subnodes */
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        node->childs[i] = optimize_tree_inner(node->childs[i], time);
    }

    /* 2. optimize this node */
    for (size_t i = 0; i < optimization_rules_len; ++i)
    {
        node = optimization_rules[i](node, time);
    }
    return node;
}


struct node_t *optimize_tree(struct node_t *node)
{
    /* go through tree, apply randomly optimizations */
    for (int i = 0; i < 100; ++i)
    {
        node = optimize_tree_inner(node, i / 100.0);
    }
    return node;
}
