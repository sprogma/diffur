#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "ctype.h"
#include "string.h"
#include "stdio.h"
#include "time.h"


struct node_t *derivative(struct node_t *node)
{
    switch (node->type)
    {
        case NODE_TYPE_OP_SUB:
        case NODE_TYPE_OP_ADD:
        case NODE_TYPE_OP_MUL:
        case NODE_TYPE_BRACES:
        case NODE_TYPE_OP_DIV:
        case NODE_TYPE_OP_POW:
        case NODE_TYPE_OP_COMMA:
        case NODE_TYPE_NULL:
            {
                printf("Error: derivative of NULL/technical node\n");
                struct node_t *res = new_node();
                res->type = NODE_TYPE_NULL;
                res->end = res->start = strdup("");
                return res;
            }
            break;
        case NODE_TYPE_ADDSUB:
            {
                struct node_t *res = new_node();
                res->type = NODE_TYPE_ADDSUB;
                res->start = node->start;
                res->end = node->end;

                add_child(res, node->childs[0]);
                add_child(res, node->childs[1]);
                add_child(res, node->childs[2]);
                
                res->childs[0] = derivative(res->childs[0]);
                res->childs[2] = derivative(res->childs[2]);
                
                return res;
            }
            break;
        case NODE_TYPE_MULDIV:
            { 
                if (node->childs[1]->type == NODE_TYPE_OP_MUL)
                {
                    struct node_t *mul1 = new_node_ex(NODE_TYPE_MULDIV, "", "");
                    struct node_t *mul1s = new_node_ex(NODE_TYPE_OP_MUL, "", "");
                    struct node_t *mul2 = new_node_ex(NODE_TYPE_MULDIV, "", "");
                    struct node_t *mul2s = new_node_ex(NODE_TYPE_OP_MUL, "", "");
                    struct node_t *add1 = new_node_ex(NODE_TYPE_ADDSUB, "", "");
                    struct node_t *add1s = new_node_ex(NODE_TYPE_OP_ADD, "", "");

                    add_child(mul1, derivative(node->childs[0]));
                    add_child(mul1, mul1s);
                    add_child(mul1, node->childs[2]);

                    add_child(mul2, node->childs[0]);
                    add_child(mul2, mul2s);
                    add_child(mul2, derivative(node->childs[2]));

                    add_child(add1, mul1);
                    add_child(add1, add1s);
                    add_child(add1, mul2);
                    
                    return add1;
                }
                else
                {
                    printf("NOT SUPPORTED\n");
                    exit(1);
                }
            }
            break;
        case NODE_TYPE_FLOAT:
            { 
                struct node_t *res = new_node();
                res->type = NODE_TYPE_FLOAT;
                res->end = res->start = strdup("0");
                return res;
            }
            break;
        case NODE_TYPE_IDENTIFER:
            { 
                if (strncmp(node->start, "x", node->end - node->start) == 0)
                { 
                    struct node_t *res = new_node();
                    res->type = NODE_TYPE_FLOAT;
                    res->end = res->start = strdup("1");
                    return res;
                }
                struct node_t *res = new_node();
                res->type = NODE_TYPE_FLOAT;
                res->end = res->start = strdup("0");
                return res;
            }
            break;
        case NODE_TYPE_POW:
            {
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
        case NODE_TYPE_FN_ARGS:
            {
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
        case NODE_TYPE_FN_CALL:
            { 
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
        case NODE_TYPE_OP_PREFIX:
            { 
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
    }
    printf("ERROR!\n");
    return NULL;
}



int main()
{
    srand(time(NULL));

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

    struct node_t *tree = result.node;

    char *buf = malloc(1024 * 64 * 64), *t = NULL;
    

    printf("Parsing completed.\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    tree = normalize_tree(tree);
    

    printf("Normalized tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    tree = optimize_tree(tree);
    

    printf("Optimized tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    printf("Exported:\n");
    t = export_basic(buf, tree);
    *t = 0;
    printf("%s\n", buf);
// 
// 
//     tree = derivative(tree);
//     
// 
//     printf("Derivative tree:\n");
//     t = export_ast(buf, tree);
//     *t = 0;
//     printf("%s\n", buf);
// 
// 
//     tree = optimize_tree(tree);
//     
// 
//     printf("Optimized derivative tree:\n");
//     t = export_ast(buf, tree);
//     *t = 0;
//     printf("%s\n", buf);
// 
// 
//     printf("Exported:\n");
//     t = export_basic(buf, tree);
//     *t = 0;
//     printf("%s\n", buf);

    free(buf);
    
    return 0;
}
