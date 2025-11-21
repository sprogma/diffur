#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "math.h"
#include "string.h"
#include "ctype.h"
#include "stdio.h"


static struct node_t *optimize_addsubpow_constants(struct node_t *node, double time)
{
    (void)time;
        
    if ((node->type == NODE_TYPE_ADDSUB ||
         node->type == NODE_TYPE_MULDIV ||
         node->type == NODE_TYPE_POW) &&
         node->childs[0]->type == NODE_TYPE_FLOAT &&
         node->childs[2]->type == NODE_TYPE_FLOAT)
    {
        char *end;
        double res = 0.0;
        switch (node->childs[1]->type)
        {
            case NODE_TYPE_OP_ADD:
                res = strtod(node->childs[0]->start, &end) + strtod(node->childs[2]->start, &end);
                break;
            case NODE_TYPE_OP_SUB:
                res = strtod(node->childs[0]->start, &end) - strtod(node->childs[2]->start, &end);
                break;
            case NODE_TYPE_OP_MUL:
                res = strtod(node->childs[0]->start, &end) * strtod(node->childs[2]->start, &end);
                break;
            case NODE_TYPE_OP_DIV:
                res = strtod(node->childs[0]->start, &end) / strtod(node->childs[2]->start, &end);
                break;
            case NODE_TYPE_OP_POW:
                res = pow(strtod(node->childs[0]->start, &end), strtod(node->childs[2]->start, &end));
                break;
            default:
                printf("Error: unknown subnode type\n");
                return node;
        }

        struct node_t *resnode = new_node();
        resnode->type = NODE_TYPE_FLOAT;
        char buf[128];
        sprintf(buf, "%g", res);
        resnode->start = strdup(buf);
        resnode->end = resnode->start + strlen(buf);
        
        // free_node(node);
        
        return resnode;
    }

    return node;
}

static struct node_t *optimize_prefix_plus(struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_OP_PREFIX &&
        node->childs[0]->type == NODE_TYPE_OP_ADD)
    {
        return node->childs[1];
    }
    if (node->type == NODE_TYPE_OP_PREFIX &&
        node->childs[1]->type == NODE_TYPE_FLOAT)
    {
        char *end;
        double res = 0.0;
        switch (node->childs[0]->type)
        {
            case NODE_TYPE_OP_ADD:
                res = + strtod(node->childs[1]->start, &end);
                break;
            case NODE_TYPE_OP_SUB:
                res = - strtod(node->childs[1]->start, &end);
                break;
            default:
                printf("Error: unknown subnode type\n");
                return node;
        }

        struct node_t *resnode = new_node();
        resnode->type = NODE_TYPE_FLOAT;
        char buf[128];
        sprintf(buf, "%g", res);
        resnode->start = strdup(buf);
        resnode->end = resnode->start + strlen(buf);
        
        // free_node(node);
        
        return resnode;
    }
    return node;
}

static struct node_t *optimize_add_with_prefix_minus(struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_ADDSUB &&
        node->childs[2]->type == NODE_TYPE_OP_PREFIX &&
        node->childs[2]->childs[0]->type == NODE_TYPE_OP_SUB)
    {
        if (node->childs[1]->type == NODE_TYPE_OP_SUB)
        {
            node->childs[1]->type = NODE_TYPE_OP_ADD;
        }
        else
        {
            node->childs[1]->type = NODE_TYPE_OP_SUB;
        }
        node->childs[2] = node->childs[2]->childs[1];
        return node;
    }
    return node;
}

static struct node_t *optimize_shuffle_ops(struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_ADDSUB || 
        node->type == NODE_TYPE_MULDIV)
    {
        if (node->childs[0]->type == node->type && rand() % 1000 > time * 1000)
        {
            /* swap operations and operands */
            struct node_t *c1 = node->childs[1], *c2 = node->childs[2];
            node->childs[1] = node->childs[0]->childs[1]; 
            node->childs[2] = node->childs[0]->childs[2]; 
            node->childs[0]->childs[1] = c1;
            node->childs[0]->childs[2] = c2;
        }
        else if (node->childs[2]->type == node->type && rand() % 1000 > time * 1000)
        {
            /* swap operations and operands */
            struct node_t *c0 = node->childs[0], *c1 = node->childs[1];

            node->childs[0] = node->childs[2];
            node->childs[2] = node->childs[2]->childs[2];
            
            node->childs[0]->childs[2] = node->childs[0]->childs[0];
            node->childs[0]->childs[0] = c0;
            node->childs[0]->childs[1] = c1;

            if (node->childs[1]->type == NODE_TYPE_OP_ADD)
            {
                node->childs[1]->type = NODE_TYPE_OP_SUB;
            }
            else if (node->childs[1]->type == NODE_TYPE_OP_SUB)
            {
                node->childs[1]->type = NODE_TYPE_OP_ADD;
            }
            else if (node->childs[1]->type == NODE_TYPE_OP_MUL)
            {
                node->childs[1]->type = NODE_TYPE_OP_DIV;
            }
            else if (node->childs[1]->type == NODE_TYPE_OP_DIV)
            {
                node->childs[1]->type = NODE_TYPE_OP_MUL;
            }
        }
        return node;
    }
    return node;
}




static struct node_t *optimize_neutral_elements(struct node_t *node, double time)
{
    (void)time;
    switch (node->type)
    {
        case NODE_TYPE_POW:
            {
                if (node->childs[0]->type == NODE_TYPE_FLOAT)
                {
                    char *end;
                    double x = strtod(node->childs[0]->start, &end);
                    if (fabs(x) < 1e-6)
                    {
                        return node->childs[0];
                    }
                    if (fabs(x - 1.0) < 1e-6)
                    {
                        return node->childs[0];
                    }
                }
                else if (node->childs[2]->type == NODE_TYPE_FLOAT)
                {
                    char *end;
                    double x = strtod(node->childs[2]->start, &end);
                    if (fabs(x - 1.0) < 1e-6)
                    {
                        return node->childs[0];
                    }
                    if (fabs(x) < 1e-6)
                    {
                        node->childs[2]->start = strdup("1");
                        node->childs[2]->end = node->childs[2]->start + 1;
                        return node->childs[2];
                    }
                }
                return node;
            }
            break;
        case NODE_TYPE_ADDSUB:
            {
                if (node->childs[1]->type == NODE_TYPE_OP_ADD)
                {
                    if (node->childs[0]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[0]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            return node->childs[2];
                        }
                    }
                    else if (node->childs[2]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[2]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            return node->childs[0];
                        }
                    }
                    return node;
                }
            }
            break;
        case NODE_TYPE_MULDIV:
            {
                if (node->childs[1]->type == NODE_TYPE_OP_MUL)
                {
                    if (node->childs[0]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[0]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            return node->childs[0];
                        }
                        if (fabs(x - 1.0) < 1e-6)
                        {
                            return node->childs[2];
                        }
                    }
                    else if (node->childs[2]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[2]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            return node->childs[2];
                        }
                        if (fabs(x - 1.0) < 1e-6)
                        {
                            return node->childs[0];
                        }
                    }
                    return node;
                }
            }
            break;
        default:
            break;
    }
    return node;
}




struct node_t *(*optimization_rules[])(struct node_t *node, double x) = {
    optimize_addsubpow_constants,
    optimize_prefix_plus,
    optimize_add_with_prefix_minus,
    optimize_shuffle_ops,
    optimize_neutral_elements,
};
const size_t optimization_rules_len = sizeof(optimization_rules)/sizeof(*optimization_rules);

