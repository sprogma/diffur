#include "parser_api.h"
#include "parser_errors.h"
#include "ctype.h"
#include "stdio.h"


void print_node(struct node_t *node, int indent)
{
    printf("%*s", indent, "");
    printf("Node type %d : [%.*s]\n", (int)node->type, (int)(node->end - node->start), node->start);
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        print_node(node->childs[i], indent + 4);
    }
}


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


char *print_tree(char *s, struct node_t *node)
{
    switch (node->type)
    {
        case NODE_TYPE_NULL:
            { 
                printf("Error: print NULL node\n");
                return s;
            }
            break;
        case NODE_TYPE_ADDSUB:
            {
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = print_tree(s, node->childs[i]);
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, ")");
                    }
                }
            }
            break;
        case NODE_TYPE_OP_ADD:
            { 
            
                s += sprintf(s, "+");
            }
            break;
        case NODE_TYPE_OP_SUB:
            { 
                s += sprintf(s, "-");
            }
            break;
        case NODE_TYPE_MULDIV:
            { 
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = print_tree(s, node->childs[i]);
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, ")");
                    }
                }
            }
            break;
        case NODE_TYPE_OP_MUL:
            { 
                s += sprintf(s, "*");
            }
            break;
        case NODE_TYPE_OP_DIV:
            { 
                s += sprintf(s, "/");
            }
            break;
        case NODE_TYPE_BRACES:
            { 
                s += sprintf(s, "(");
                s = print_tree(s, node->childs[0]);
                s += sprintf(s, ")");
            }
            break;
        case NODE_TYPE_FLOAT:
            { 
                s += sprintf(s, "%.*s", (int)(node->end - node->start), node->start);
            }
            break;
        case NODE_TYPE_IDENTIFER:
            { 
                s += sprintf(s, "%.*s", (int)(node->end - node->start), node->start);
            }
            break;
        case NODE_TYPE_POW:
            { 
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = print_tree(s, node->childs[i]);
                    if (priority[node->childs[i]->type] < priority[node->type])
                    {
                        s += sprintf(s, ")");
                    }
                }
            }
            break;
        case NODE_TYPE_OP_POW:
            { 
                s += sprintf(s, "^");
            }
            break;
        case NODE_TYPE_OP_COMMA:
            { 
                s += sprintf(s, ",");
            }
            break;
        case NODE_TYPE_FN_ARGS:
            { 
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    s = print_tree(s, node->childs[i]);
                }
            }
            break;
        case NODE_TYPE_FN_CALL:
            { 
                s = print_tree(s, node->childs[0]);
                s += sprintf(s, "(");
                s = print_tree(s, node->childs[1]);
                s += sprintf(s, ")");
            }
            break;
        case NODE_TYPE_OP_PREFIX:
            { 
                s = print_tree(s, node->childs[0]);
                s = print_tree(s, node->childs[1]);
            }
            break;
    }
    return s;
}


int main()
{
    char s[512];
    printf("Enter string to analyze >\n");
    fgets(s, sizeof(s), stdin);
    printf("Analyzing <%s>\n", s);
    
    struct parser_error_table_t err;
    struct parse_result_t result = parse_all(s, &err);

    if (result.rest == NULL || *result.rest != 0)
    {
        printf("Parsing not completed. Syntax error.\n");

        print_errors(stderr, &result, &err);
        
        return 1;
    }

    printf("Parsing completed.\n");

    print_node(result.node, 0);

    /* remove all braces nodes */
    
    struct node_t *tree = normalize_tree(result.node);
    
    printf("Simplified tree:\n");
    print_node(tree, 0);

    printf("Expr:\n");
    char *buf = malloc(1024 * 64);
    char *t = print_tree(buf, tree);
    *t = 0;
    printf("%s\n", buf);
    free(buf);
    return 0;
}
