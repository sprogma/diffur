#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "ctype.h"
#include "stdio.h"


struct node_t *normalize_tree(struct node_t *node)
{
    if (node->type == NODE_TYPE_BRACES)
    {
        return normalize_tree(node->childs[0]);
    }
    if (node->type == NODE_TYPE_ADDSUB || 
        node->type == NODE_TYPE_MULDIV || 
        node->type == NODE_TYPE_POW)
    {
        if (node->childs_length > 3)
        {
            /* create subnode */
            struct node_t *newnode = new_node();
            newnode->type = node->type;
            newnode->start = node->start;
            newnode->end = node->end;

            
            /* remove 2 last nodes */
            if (node->type == NODE_TYPE_ADDSUB ||
                node->type == NODE_TYPE_MULDIV)
            {
                node->end = node->childs[node->childs_length - 2]->start;
                
                /* left to right nodes */
                struct node_t *y = remove_child(node, node->childs_length - 1);
                struct node_t *x = remove_child(node, node->childs_length - 1);

                add_child(newnode, normalize_tree(node));
                add_child(newnode, normalize_tree(x));
                add_child(newnode, normalize_tree(y));
            }
            else
            {
                node->start = node->childs[2]->start;
                
                /* right to left nodes */
                struct node_t *x = remove_child(node, 0);
                struct node_t *y = remove_child(node, 0);

                add_child(newnode, normalize_tree(x));
                add_child(newnode, normalize_tree(y));
                add_child(newnode, normalize_tree(node));
            }

            return newnode;
        }
    }
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        node->childs[i] = normalize_tree(node->childs[i]);
    }
    return node;
}
