#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "math.h"
#include "string.h"
#include "ctype.h"
#include "stdio.h"


static struct node_t *optimize_addsubpow_constants(const struct node_t *node, double time)
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
                return (struct node_t *)node;
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

    return (struct node_t *)node;
}

static struct node_t *optimize_prefix_plus(const struct node_t *node, double time)
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
                return (struct node_t *)node;
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
    return (struct node_t *)node;
}

static struct node_t *optimize_add_with_prefix_minus(const struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_ADDSUB &&
        node->childs[2]->type == NODE_TYPE_OP_PREFIX &&
        node->childs[2]->childs[0]->type == NODE_TYPE_OP_SUB)
    {
        struct node_t *res_node = soft_copy(node);
        
        if (res_node->childs[1]->type == NODE_TYPE_OP_SUB)
        {
            res_node->childs[1]->type = NODE_TYPE_OP_ADD;
        }
        else
        {
            res_node->childs[1]->type = NODE_TYPE_OP_SUB;
        }
        res_node->childs[2] = res_node->childs[2]->childs[1];
        return res_node;
    }
    return (struct node_t *)node;
}

static struct node_t *optimize_shuffle_ops(const struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_ADDSUB ||
        node->type == NODE_TYPE_MULDIV)
    {
        // a b c
        // b a c
        if (node->childs[0]->type == node->type && rand() % 1000 > 800)
        {
            struct node_t *res_node = soft_copy(node);
            struct node_t *leftnode = soft_copy(node->childs[0]);
            /* swap operations and operands */
            res_node->childs[0] = leftnode;
            res_node->childs[1] = node->childs[0]->childs[1]; 
            res_node->childs[2] = node->childs[0]->childs[2]; 
            leftnode->childs[1] = node->childs[1];
            leftnode->childs[2] = node->childs[2];
            
            return res_node;
        }
        // a c b
        if (node->childs[2]->type == node->type && rand() % 1000 > 750)
        {
            struct node_t *res_node = soft_copy(node);
            struct node_t *leftnode = soft_copy(node->childs[2]);
            struct node_t *leftnodeop = soft_copy(node->childs[2]->childs[1]);

            res_node->childs[0] = leftnode;
            res_node->childs[2] = node->childs[2]->childs[0];
            
            leftnode->childs[0] = node->childs[0];
            leftnode->childs[1] = leftnodeop;
            leftnode->childs[2] = node->childs[2]->childs[2];

            /* swap operations and operands */
            if (node->childs[1]->type == NODE_TYPE_OP_SUB || 
                node->childs[1]->type == NODE_TYPE_OP_DIV)
            {
                if (leftnodeop->type == NODE_TYPE_OP_ADD)
                {
                    leftnodeop->type = NODE_TYPE_OP_SUB;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_SUB)
                {
                    leftnodeop->type = NODE_TYPE_OP_ADD;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_MUL)
                {
                    leftnodeop->type = NODE_TYPE_OP_DIV;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_DIV)
                {
                    leftnodeop->type = NODE_TYPE_OP_MUL;
                }
            }
            return res_node;
        }
        // a +- (b +- c) -> (a +- b) +-/-+ c
        if (node->childs[2]->type == node->type && rand() % 1000 > 650)
        {
            struct node_t *res_node = soft_copy(node);
            struct node_t *leftnode = soft_copy(node->childs[2]);
            struct node_t *leftnodeop = soft_copy(node->childs[2]->childs[1]);

            leftnode->childs[0] = node->childs[0];
            leftnode->childs[1] = node->childs[1];
            leftnode->childs[2] = node->childs[2]->childs[0];

            res_node->childs[0] = leftnode;
            res_node->childs[1] = leftnodeop;
            res_node->childs[2] = node->childs[2]->childs[2];
            

            /* swap operations and operands */
            if (node->childs[1]->type == NODE_TYPE_OP_SUB || 
                node->childs[1]->type == NODE_TYPE_OP_DIV)
            {
                if (leftnodeop->type == NODE_TYPE_OP_ADD)
                {
                    leftnodeop->type = NODE_TYPE_OP_SUB;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_SUB)
                {
                    leftnodeop->type = NODE_TYPE_OP_ADD;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_MUL)
                {
                    leftnodeop->type = NODE_TYPE_OP_DIV;
                }
                else if (leftnodeop->type == NODE_TYPE_OP_DIV)
                {
                    leftnodeop->type = NODE_TYPE_OP_MUL;
                }
            }
            return res_node;
        }
        // (a +- b) +- c -> a +- (b +- c)
        if (node->childs[0]->type == node->type && rand() % 1000 > 500)
        {
            struct node_t *res_node = soft_copy(node);
            struct node_t *rightnode = soft_copy(node->childs[0]);
            struct node_t *rightnodeop = soft_copy(node->childs[1]);

            rightnode->childs[0] = node->childs[0]->childs[2];
            rightnode->childs[1] = rightnodeop;
            rightnode->childs[2] = node->childs[2];

            res_node->childs[0] = node->childs[0]->childs[0];
            res_node->childs[1] = node->childs[0]->childs[1];
            res_node->childs[2] = rightnode;

            /* swap operations and operands */
            if (node->childs[0]->childs[1]->type == NODE_TYPE_OP_SUB || 
                node->childs[0]->childs[1]->type == NODE_TYPE_OP_DIV)
            {
                if (rightnodeop->type == NODE_TYPE_OP_ADD)
                {
                    rightnodeop->type = NODE_TYPE_OP_SUB;
                }
                else if (rightnodeop->type == NODE_TYPE_OP_SUB)
                {
                    rightnodeop->type = NODE_TYPE_OP_ADD;
                }
                else if (rightnodeop->type == NODE_TYPE_OP_MUL)
                {
                    rightnodeop->type = NODE_TYPE_OP_DIV;
                }
                else if (rightnodeop->type == NODE_TYPE_OP_DIV)
                {
                    rightnodeop->type = NODE_TYPE_OP_MUL;
                }
            }
            return res_node;
        }
    }
    return (struct node_t *)node;
}


static struct node_t *optimize_neutral_elements(const struct node_t *node, double time)
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
                        struct node_t *res_node = soft_copy(node);
                        res_node->start = strdup("1");
                        res_node->end = node->childs[2]->start + 1;
                        res_node->type = NODE_TYPE_FLOAT;
                        return res_node;
                    }
                }
                return (struct node_t *)node;
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
                    return (struct node_t *)node;
                }
                else /* - */
                {
                    if (node->childs[0]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[0]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            struct node_t *res_node = new_node_ex(NODE_TYPE_OP_PREFIX, NULL, NULL);
                            struct node_t *sign_node = new_node_ex(NODE_TYPE_OP_SUB, NULL, NULL);

                            add_child(res_node, sign_node);
                            add_child(res_node, node->childs[2]);
                            
                            return res_node;
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
                    return (struct node_t *)node;
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
                    return (struct node_t *)node;
                }
                else /* / */
                {
                    if (node->childs[0]->type == NODE_TYPE_FLOAT)
                    {
                        char *end;
                        double x = strtod(node->childs[0]->start, &end);
                        if (fabs(x) < 1e-6)
                        {
                            return node->childs[0];
                        }
                        if (fabs(x - 1.0) < 1e-6 && rand() % 3 == 0)
                        {
                            struct node_t *res_node = new_node_ex(NODE_TYPE_POW, NULL, NULL);
                            struct node_t *sign_node = new_node_ex(NODE_TYPE_OP_POW, NULL, NULL);
                            struct node_t *minus1 = new_node_ex(NODE_TYPE_FLOAT, NULL, NULL);
                            
                            minus1->start = strdup("-1");
                            minus1->end = minus1->start + 2;

                            add_child(res_node, node->childs[2]);
                            add_child(res_node, sign_node);
                            add_child(res_node, minus1);
                            
                            return res_node;
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
                    return (struct node_t *)node;
                }
            }
            break;
        default:
            break;
    }
    return (struct node_t *)node;
}

struct node_t *optimize_simple_sums(const struct node_t *node, double time)
{
    if (node->type == NODE_TYPE_ADDSUB)
    {
        if (rand() % 1000 < time * 1000)
        {
            if (is_same(node->childs[0], node->childs[2]))
            {
                if (node->childs[1]->type == NODE_TYPE_OP_ADD)
                {
                    struct node_t *res_node = new_node();
                    res_node->type = NODE_TYPE_MULDIV;
                    
                    struct node_t *node2 = new_node();
                    node2->type = NODE_TYPE_FLOAT;
                    node2->start = strdup("2");
                    node2->end = node2->start + 1;
                    
                    struct node_t *nodeop = new_node();
                    nodeop->type = NODE_TYPE_OP_MUL;
                    nodeop->start = strdup("*");
                    nodeop->end = node2->start + 1;

                    add_child(res_node, node2);
                    add_child(res_node, nodeop);
                    add_child(res_node, node->childs[0]);
                    
                    return res_node;
                }
                else
                {
                    struct node_t *res_node = new_node();
                    res_node->type = NODE_TYPE_FLOAT;
                    res_node->start = strdup("0");
                    res_node->end = res_node->start + 1;

                    return res_node;
                }
            }
        }
        if (node->childs[0]->type == NODE_TYPE_MULDIV && node->childs[0]->childs[1]->type == NODE_TYPE_OP_MUL &&
            node->childs[2]->type == NODE_TYPE_MULDIV && node->childs[2]->childs[1]->type == NODE_TYPE_OP_MUL &&
            rand() % 1000 < time * 1000)
        {
            struct node_t
            if (is_same(node->childs[0]->childs[0], node->childs[2]->childs[0]))
            {
                struct node_t *res_node = new_node_ex(NODE_TYPE_MULDIV, NULL, NULL);
                struct node_t *addnode = new_node_ex(NODE_TYPE_ADDSUB, NULL, NULL);
                struct node_t *addop = new_node_ex(NODE_TYPE_OP_ADD, NULL, NULL);

                add_child(addnode, node->childs[0]->childs[2]);
                add_child(addnode, addop);
                add_child(addnode, node->childs[2]->childs[2]);

                add_child(res_node, node->childs[0]->childs[0]);
                add_child(res_node, node->childs[0]->childs[1]);
                add_child(res_node, addnode);
            
                return res_node;
            }
        }
        if ((node->childs[0]->type == NODE_TYPE_MULDIV && node->childs[0]->childs[1]->type == NODE_TYPE_OP_MUL) || 
            (node->childs[2]->type == NODE_TYPE_MULDIV && node->childs[2]->childs[1]->type == NODE_TYPE_OP_MUL))
        {
            struct node_t *mul = NULL;
            struct node_t *other = NULL;
            if (node->childs[0]->type == NODE_TYPE_MULDIV && node->childs[0]->childs[1]->type == NODE_TYPE_OP_MUL)
            {
                mul = node->childs[0];
                other = node->childs[2];
            }
            else
            {
                mul = node->childs[2];
                other = node->childs[0];
            }
            struct node_t *same = NULL, *coeff = NULL;

            if (is_same(mul->childs[0], other))
            {
                same = mul->childs[0];
                coeff = mul->childs[2];
            }
            else if (is_same(mul->childs[2], other))
            {
                same = mul->childs[2];
                coeff = mul->childs[0];
            }
            
            if (same && coeff)
            {
                struct node_t *res_node = new_node();
                res_node->type = NODE_TYPE_MULDIV;
                
                struct node_t *node_add = new_node();
                node_add->type = NODE_TYPE_ADDSUB;
                node_add->start = node_add->start = strdup("");
                
                struct node_t *node2 = new_node();
                node2->type = NODE_TYPE_FLOAT;
                node2->start = strdup("1");
                node2->end = node2->start + 1;
                
                struct node_t *node_add_op = new_node();
                node_add_op->type = (node->childs[1]->type == NODE_TYPE_OP_ADD ? NODE_TYPE_OP_ADD : NODE_TYPE_OP_SUB);
                node_add_op->end = node_add_op->start = NULL;

                add_child(node_add, coeff);
                add_child(node_add, node_add_op);
                add_child(node_add, node2);
                
                struct node_t *nodeop = new_node();
                nodeop->type = NODE_TYPE_OP_MUL;
                nodeop->start = strdup("*");
                nodeop->end = node2->start + 1;

                add_child(res_node, same);
                add_child(res_node, nodeop);
                add_child(res_node, node_add);
                
                return res_node;
            }
        }
    }
    
    return (struct node_t *)node;
}


struct node_t *optimize_simple_muls(const struct node_t *node, double time)
{
    if (node->type == NODE_TYPE_MULDIV)
    {
        if (rand() % 1000 < time * 1000)
        {
            if (is_same(node->childs[0], node->childs[2]))
            {
                if (node->childs[1]->type == NODE_TYPE_OP_MUL)
                {
                    struct node_t *res_node = new_node();
                    res_node->type = NODE_TYPE_POW;
                    
                    struct node_t *node2 = new_node();
                    node2->type = NODE_TYPE_FLOAT;
                    node2->start = strdup("2");
                    node2->end = node2->start + 1;
                    
                    struct node_t *nodeop = new_node();
                    nodeop->type = NODE_TYPE_OP_POW;
                    nodeop->start = strdup("^");
                    nodeop->end = node2->start + 1;

                    add_child(res_node, node->childs[0]);
                    add_child(res_node, nodeop);
                    add_child(res_node, node2);
                    
                    return res_node;
                }
                else
                {
                    struct node_t *res_node = new_node();
                    res_node->type = NODE_TYPE_FLOAT;
                    res_node->start = strdup("1");
                    res_node->end = res_node->start + 1;

                    return res_node;
                }
            }
        }
        if (node->childs[0]->type == NODE_TYPE_POW &&
            node->childs[2]->type == NODE_TYPE_POW &&
            is_same(node->childs[0]->childs[0], node->childs[2]->childs[0]) &&
            rand() % 1000 < time * 1000)
        {
            struct node_t *res_node = new_node_ex(NODE_TYPE_POW, NULL, NULL);
            struct node_t *addnode = new_node_ex(NODE_TYPE_ADDSUB, NULL, NULL);
            struct node_t *addop = new_node_ex(NODE_TYPE_OP_ADD, NULL, NULL);

            add_child(addnode, node->childs[0]->childs[2]);
            add_child(addnode, addop);
            add_child(addnode, node->childs[2]->childs[2]);

            add_child(res_node, node->childs[0]->childs[0]);
            add_child(res_node, node->childs[0]->childs[1]);
            add_child(res_node, addnode);
        
            return res_node;
        }
        if (node->childs[0]->type == NODE_TYPE_POW || 
            node->childs[2]->type == NODE_TYPE_POW)
        {
            struct node_t *pow = NULL;
            struct node_t *other = NULL;
            if (node->childs[0]->type == NODE_TYPE_POW)
            {
                pow = node->childs[0];
                other = node->childs[2];
            }
            else
            {
                pow = node->childs[2];
                other = node->childs[0];
            }

            if (is_same(pow->childs[0], other))
            {
                struct node_t *res_node = new_node();
                res_node->type = NODE_TYPE_POW;
                
                struct node_t *node_add = new_node();
                node_add->type = NODE_TYPE_ADDSUB;
                node_add->start = node_add->start = strdup("");
                
                struct node_t *node2 = new_node();
                node2->type = NODE_TYPE_FLOAT;
                node2->start = strdup("1");
                node2->end = node2->start + 1;
                
                struct node_t *node_add_op = new_node();
                node_add_op->type = (node->childs[1]->type == NODE_TYPE_OP_MUL ? NODE_TYPE_OP_ADD : NODE_TYPE_OP_SUB);
                node_add_op->end = node_add_op->start = NULL;

                add_child(node_add, pow->childs[2]);
                add_child(node_add, node_add_op);
                add_child(node_add, node2);
                
                struct node_t *nodeop = new_node();
                nodeop->type = NODE_TYPE_OP_POW;
                nodeop->start = strdup("^");
                nodeop->end = node2->start + 1;

                add_child(res_node, pow->childs[0]);
                add_child(res_node, nodeop);
                add_child(res_node, node_add);
                
                return res_node;
            }
        }
    }
    
    return (struct node_t *)node;
}


struct node_t *optimize_simple_pows(const struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_POW)
    {
        if (node->childs[2]->type == NODE_TYPE_FLOAT)
        {
            char *e;
            double pow = strtod(node->childs[2]->start, &e);

            if (fabs(pow) < 1e-6)
            {
                char *s = strdup("1");
                struct node_t *fltnode = new_node_ex(NODE_TYPE_FLOAT, s, s + 1);
                
                return fltnode;
            }
            if (fabs(pow - 1.0) < 1e-6)
            {
                return node->childs[0];
            }
            if (fabs(pow + 1.0) < 1e-6)
            {
                /* 1/x */
                struct node_t *mulnode = new_node_ex(NODE_TYPE_MULDIV, "", "");
                struct node_t *mulnodeop = new_node_ex(NODE_TYPE_OP_DIV, "", "");

                char *s = strdup("1");
                struct node_t *fltnode = new_node_ex(NODE_TYPE_FLOAT, s, s + 1);
                add_child(mulnode, fltnode);
                add_child(mulnode, mulnodeop);
                add_child(mulnode, node->childs[0]);
                
                return mulnode;
            }
        }
    }
    
    return (struct node_t *)node;
}


struct node_t *deoptimize_pow(const struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_POW)
    {
        if (node->childs[2]->type == NODE_TYPE_FLOAT)
        {
            char *e;
            double pow = strtod(node->childs[2]->start, &e);
            
            struct node_t *res_node = soft_copy(node);

            /* decrease power */
            char new_s[128];
            int64_t len = sprintf(new_s, "%g", pow - 1.0);
            char *s = strdup(new_s);
            struct node_t *fltnode = new_node_ex(NODE_TYPE_FLOAT, s, s + len);
            res_node->childs[2] = fltnode;

            /* mul on body */

            struct node_t *mulnode = new_node_ex(NODE_TYPE_MULDIV, "", "");
            struct node_t *mulnodeop = new_node_ex(NODE_TYPE_OP_MUL, "", "");

            add_child(mulnode, node->childs[0]);
            add_child(mulnode, mulnodeop);
            add_child(mulnode, res_node);
            
            return mulnode;
        }
    }
    
    return (struct node_t *)node;
}


struct node_t *deoptimize_mul(const struct node_t *node, double time)
{
    (void)time;
    
    if (node->type == NODE_TYPE_MULDIV && 
        node->childs[1]->type == NODE_TYPE_OP_MUL)
    {
        struct node_t *addsub = NULL, *other = NULL;
        if (node->childs[0]->type == NODE_TYPE_ADDSUB)
        {
            addsub = node->childs[0];
            other = node->childs[2];
        }
        if (node->childs[2]->type == NODE_TYPE_ADDSUB && (!addsub || rand() % 2))
        {
            addsub = node->childs[2];
            other = node->childs[0];
        }
        if (addsub != NULL)
        {
            /*  x * (a +- b)  */
            /*  x * a +- x * b  */
            struct node_t *res_node = new_node_ex(NODE_TYPE_ADDSUB, NULL, NULL);
            struct node_t *mul1 = new_node_ex(NODE_TYPE_MULDIV, NULL, NULL);
            struct node_t *mul2 = new_node_ex(NODE_TYPE_MULDIV, NULL, NULL);

            add_child(mul1, other);
            add_child(mul1, node->childs[1]);
            add_child(mul1, addsub->childs[0]);

            add_child(mul2, other);
            add_child(mul2, node->childs[1]);
            add_child(mul2, addsub->childs[2]);
            
            add_child(res_node, mul1);
            add_child(res_node, addsub->childs[1]);
            add_child(res_node, mul2);

            return res_node;
        }
    }
    
    return (struct node_t *)node;
}




struct node_t *(*optimization_rules[])(const struct node_t *node, double x) = {
    optimize_addsubpow_constants,
    optimize_prefix_plus,
    optimize_add_with_prefix_minus,
    optimize_shuffle_ops,
    optimize_neutral_elements,
    optimize_simple_sums,
    optimize_simple_muls,
    optimize_simple_pows,
    deoptimize_pow,
    deoptimize_mul,
};
const size_t optimization_rules_len = sizeof(optimization_rules)/sizeof(*optimization_rules);
