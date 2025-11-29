#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "math.h"
#include "string.h"
#include "ctype.h"
#include "stdio.h"


int is_same(struct node_t *a, struct node_t *b)
{
    if (a->type != b->type)
    {
        return 0;
    }
    if (a->type == NODE_TYPE_FLOAT)
    {
        char *e;
        double x = strtod(a->start, &e);
        double y = strtod(b->start, &e);
        return fabs(x - y) < 1e-6;
    }
    if (a->type == NODE_TYPE_IDENTIFER)
    {
        if (a->end - a->start != b->end - b->start)
        {
            return 0;
        }
        return strncmp(a->start, b->start, a->end - a->start) == 0;
    }
    if (a->childs_length != b->childs_length)
    {
        return 0;
    }
    if (a->type == NODE_TYPE_ADDSUB && 
        a->childs[1]->type == NODE_TYPE_OP_ADD && 
        b->childs[1]->type == NODE_TYPE_OP_ADD)
    {
        return (is_same(a->childs[0], b->childs[0]) && is_same(a->childs[2], b->childs[2])) ||
               (is_same(a->childs[0], b->childs[2]) && is_same(a->childs[2], b->childs[0]));
    }
    if (a->type == NODE_TYPE_MULDIV && 
        a->childs[1]->type == NODE_TYPE_OP_MUL && 
        b->childs[1]->type == NODE_TYPE_OP_MUL)
    {
        return (is_same(a->childs[0], b->childs[0]) && is_same(a->childs[2], b->childs[2])) ||
               (is_same(a->childs[0], b->childs[2]) && is_same(a->childs[2], b->childs[0]));
    }
    for (size_t i = 0; i < a->childs_length; ++i)
    {
        if (!is_same(a->childs[i], b->childs[i]))
        {
            return 0;
        }
    }
    return 1;
}


double optimization_cost(struct node_t *node)
{
    double res = 1.0;
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        int sign = 0;
        if (node->type == NODE_TYPE_ADDSUB ||
            node->type == NODE_TYPE_MULDIV ||
            node->type == NODE_TYPE_POW)
        {    
             sign = (node->childs[1]->type == NODE_TYPE_OP_MUL) ||
                   (node->childs[1]->type == NODE_TYPE_OP_ADD);
        }
        double ch_res = optimization_cost(node->childs[i]);
        if (node->type == NODE_TYPE_ADDSUB ||
            node->type == NODE_TYPE_MULDIV ||
            node->type == NODE_TYPE_POW)
        {
            if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
            {
                ch_res *= 1.1;
                ch_res += 2.0;
            }
        }
        res += ch_res;
    }
    if (node->type == NODE_TYPE_POW)
    {
        res *= 0.95;
    }
    if (node->type == NODE_TYPE_MULDIV)
    {
        res *= 1.1;
    }
    if (node->type == NODE_TYPE_MULDIV && node->childs[1]->type == NODE_TYPE_OP_DIV)
    {
        res += 13.0;
    }
    if (node->type == NODE_TYPE_ADDSUB && node->childs[1]->type == NODE_TYPE_OP_SUB)
    {
        res += 2.5;
    }
    return res;
}



struct node_t *optimize_tree_inner(struct node_t *node, double time, int too_big)
{
    struct node_t *curr_node = soft_copy(node);
    /* 1. optimize subnodes */
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        struct node_t *x = optimize_tree_inner(node->childs[i], time, too_big);
        curr_node->childs[i] = x;
    }

    /* 2. optimize this node */
    for (size_t i = 0; i < optimization_rules_len; ++i)
    {
        // char buf[10000];
        // 
        // char *t = export_basic(buf, node);
        // *t = 0;
        
        struct node_t *result = optimization_rules[i](curr_node, time);

        // if (!is_same(result, node))
        // {
        //     printf("[ after applying of %lld ]WAS:\n", i);
        //     printf("%s\n", buf);
        //     printf("Cost: %g\n", optimization_cost(node));
        //     
        //     /* print */
        //     printf("[ after applying of %lld ]NOW:\n", i);
        //     t = export_basic(buf, result);
        //     *t = 0;
        //     printf("%s\n", buf);
        //     printf("Cost: %g\n", optimization_cost(result));
        // }
        
        /* measure profit */
        double was = optimization_cost(curr_node);
        double now = optimization_cost(result);
        double p = 0.8 * exp(time * (was - now) / 0.5);
        if (was < now)
        {
            p *= 0.05;
        }
        if (time > 0.6)
        {
            p = time * (now - 1e-4 <= was);
        }
        if (too_big)
        {
            p = (now - 1e-4 <= was);
        }
        if (rand() / (RAND_MAX + 1.0) < p)
        {
            curr_node = result;
        }
    }
    return curr_node;
}


struct node_t *optimize_tree(struct node_t *node, int N)
{
    /* go through tree, apply randomly optimizations */
<<<<<<< HEAD
    struct node_t *best = node;
    double value = optimization_cost(best);
=======
    for (int i = 0; i < N; ++i)
    {
        double res = optimization_cost(node);
        struct node_t *res_node = optimize_tree_inner(node, i / (double)N, res > 5000.0);
>>>>>>> aae245cc81c8f2b06008bdaf7bf782378bf5091a

    for (int t = 0; t < 60; ++t)
    {
        for (int i = 0; i < 400; ++i)
        {
<<<<<<< HEAD
            struct node_t *res_node = optimize_tree_inner(node, i / 100.0);

            // if (!is_same(res_node, node))
            // {
            //     char buf[10000];
            //     printf("Exported:\n");
            //     char *t = export_basic(buf, node);
            //     *t = 0;
            //     printf("%s\n", buf);
            //     printf("Cost: %g\n", optimization_cost(node));
            // }
            // else
            // {
            //     printf("No changes...\n");
            // }
            
            node = res_node;
=======
            double cost = optimization_cost(res_node);

            double p = exp((0.1 + i / (double)N) * (res - cost) / 10.0);
            printf("P=%g: %g->%g\n", p, res, cost);
            if (rand() / (RAND_MAX + 1.0) < p)
            {
                char buf[100000];
                printf("Exported:\n");
                char *t = export_basic(buf, node);
                *t = 0;
                printf("%s\n", buf);
                printf("Cost: %g\n", cost);
                node = res_node;
            }
>>>>>>> aae245cc81c8f2b06008bdaf7bf782378bf5091a
        }

        double cost = optimization_cost(node);

        if (cost < value)
        {
            best = node;
            value = cost;
        }
    }
    
    return best;
}
