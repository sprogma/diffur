#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "ctype.h"
#include "stdio.h"


char *export_basic(char *s, struct node_t *node)
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
                int sign = node->childs[1]->type == NODE_TYPE_OP_ADD;
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = export_basic(s, node->childs[i]);
                    if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
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
                int sign = node->childs[1]->type == NODE_TYPE_OP_MUL;
                for (size_t i = 0; i < node->childs_length; i++)
                {
                    if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = export_basic(s, node->childs[i]);
                    if (priority[node->childs[i]->type] + (sign || i == 0) <= priority[node->type])
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
                s = export_basic(s, node->childs[0]);
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
                    if (priority[node->childs[i]->type] <= priority[node->type])
                    {
                        s += sprintf(s, "(");
                    }
                    s = export_basic(s, node->childs[i]);
                    if (priority[node->childs[i]->type] <= priority[node->type])
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
                    s = export_basic(s, node->childs[i]);
                }
            }
            break;
        case NODE_TYPE_FN_CALL:
            { 
                s = export_basic(s, node->childs[0]);
                s += sprintf(s, "(");
                s = export_basic(s, node->childs[1]);
                s += sprintf(s, ")");
            }
            break;
        case NODE_TYPE_OP_PREFIX:
            { 
                s = export_basic(s, node->childs[0]);
                if (node->childs[1]->type == NODE_TYPE_ADDSUB)
                {
                    s += sprintf(s, "(");
                }
                s = export_basic(s, node->childs[1]);
                if (node->childs[1]->type == NODE_TYPE_ADDSUB)
                {
                    s += sprintf(s, ")");
                }
            }
            break;
    }
    return s;
}


char *export_ast_inner(char *s, struct node_t *node, int indent)
{
    s += sprintf(s, "%*s", indent, "");
    s += sprintf(s, "Node type %d : [%.*s]\n", (int)node->type, (int)(node->end - node->start), node->start);
    
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        s = export_ast_inner(s, node->childs[i], indent + 4);
    }
    return s;
}

char *export_ast(char *s, struct node_t *node)
{
    return export_ast_inner(s, node, 0);
}

