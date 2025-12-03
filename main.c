#include "parser_api.h"
#include "parser_errors.h"
#include "export.h"
#include "astworker.h"
#include "ctype.h"
#include "string.h"
#include "stdio.h"
#include "time.h"


struct node_t *ADD(struct node_t *a, struct node_t *b)
{
    struct node_t *res = new_node_ex(NODE_TYPE_ADDSUB, "", "");
    struct node_t *op = new_node_ex(NODE_TYPE_OP_ADD, "", "");

    add_child(res, a);
    add_child(res, op);
    add_child(res, b);

    return res;
}

struct node_t *SUB(struct node_t *a, struct node_t *b)
{
    struct node_t *res = new_node_ex(NODE_TYPE_ADDSUB, "", "");
    struct node_t *op = new_node_ex(NODE_TYPE_OP_SUB, "", "");

    add_child(res, a);
    add_child(res, op);
    add_child(res, b);

    return res;
}

struct node_t *MUL(struct node_t *a, struct node_t *b)
{
    struct node_t *res = new_node_ex(NODE_TYPE_MULDIV, "", "");
    struct node_t *op = new_node_ex(NODE_TYPE_OP_MUL, "", "");

    add_child(res, a);
    add_child(res, op);
    add_child(res, b);

    return res;
}

struct node_t *DIV(struct node_t *a, struct node_t *b)
{
    struct node_t *res = new_node_ex(NODE_TYPE_MULDIV, "", "");
    struct node_t *op = new_node_ex(NODE_TYPE_OP_DIV, "", "");

    add_child(res, a);
    add_child(res, op);
    add_child(res, b);

    return res;
}

struct node_t *POW(struct node_t *a, struct node_t *b)
{
    struct node_t *res = new_node_ex(NODE_TYPE_POW, "", "");
    struct node_t *op = new_node_ex(NODE_TYPE_OP_POW, "", "");

    add_child(res, a);
    add_child(res, op);
    add_child(res, b);

    return res;
}

struct node_t *LOG(struct node_t *a)
{
    char *s = strdup("ln");
    struct node_t *res = new_node_ex(NODE_TYPE_FN_CALL, "", "");
    struct node_t *fn = new_node_ex(NODE_TYPE_IDENTIFER, s, s + strlen(s));
    struct node_t *args = new_node_ex(NODE_TYPE_FN_ARGS, "", "");

    add_child(res, fn);
    add_child(res, args);
    
    add_child(args, a);

    return res;
}


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
                struct node_t *res = soft_copy(node);
                
                res->childs[0] = derivative(res->childs[0]);
                res->childs[2] = derivative(res->childs[2]);
                
                return res;
            }
            break;
        case NODE_TYPE_MULDIV:
            { 
                if (node->childs[1]->type == NODE_TYPE_OP_MUL)
                {
                    return
                    ADD(
                        MUL(
                            derivative(node->childs[0]),
                            node->childs[2]
                        ),
                        MUL(
                            derivative(node->childs[2]),
                            node->childs[0]
                        )
                    );
                }
                else
                {
                    char *s = strdup("2");
                    return 
                    DIV(
                        SUB(
                            MUL(
                                derivative(node->childs[0]),
                                node->childs[2]
                            ),
                            MUL(
                                derivative(node->childs[2]),
                                node->childs[0]
                            )
                        ),
                        POW(
                            node->childs[2],
                            new_node_ex(NODE_TYPE_FLOAT, s, s + 1)
                        )
                    );
                }
            }
            break;
        case NODE_TYPE_FLOAT:
            { 
                struct node_t *res = new_node();
                res->type = NODE_TYPE_FLOAT;
                res->end = (res->start = strdup("0")) + 1;
                return res;
            }
            break;
        case NODE_TYPE_IDENTIFER:
            { 
                if (strncmp(node->start, "x", node->end - node->start) == 0)
                { 
                    struct node_t *res = new_node();
                    res->type = NODE_TYPE_FLOAT;
                    res->end = (res->start = strdup("1")) + 1;
                    return res;
                }
                struct node_t *res = new_node();
                res->type = NODE_TYPE_FLOAT;
                res->end = (res->start = strdup("0")) + 1;
                return res;
            }
            break;
        case NODE_TYPE_POW:
            {
                char *s = strdup("1");
                return 
                ADD(
                    MUL(
                        MUL(
                            node->childs[2], 
                            POW(
                                node->childs[0], 
                                SUB(node->childs[2], new_node_ex(NODE_TYPE_FLOAT, s, s + 1))
                            )
                        ),
                        derivative(node->childs[0])
                    ),
                    MUL(
                        LOG(node->childs[0]),
                        MUL(
                            POW(
                                node->childs[0],
                                node->childs[2]
                            ),
                            derivative(node->childs[2])
                        )
                    )
                );
            }
            break;

            optimize (Add (Var v) (Sub (Var v) (Imm 0)) = Add 5 5
            
        case NODE_TYPE_FN_ARGS:
            {
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
        case NODE_TYPE_FN_CALL:
            { 
                if (strncmp(node->childs[0]->start, "ln", node->childs[0]->end - node->childs[0]->start) == 0)
                {
                    char *s = strdup("1");
                    return 
                    MUL(
                        DIV(
                            new_node_ex(NODE_TYPE_FLOAT, s, s + 1),
                            node->childs[1]->childs[0]
                        ),
                        derivative(node->childs[1]->childs[0])
                    );
                }
                if (strncmp(node->childs[0]->start, "exp", node->childs[0]->end - node->childs[0]->start) == 0)
                {
                    return 
                    MUL(
                        derivative(node->childs[1]->childs[0]),
                        node
                    );
                }
                printf("NOT SUPPORTED\n");
                exit(1);
            }
            break;
        case NODE_TYPE_OP_PREFIX:
            {
                if (node->childs[0]->type == NODE_TYPE_OP_SUB)
                {
                    struct node_t *res = soft_copy(node);
                    res->childs[1] = derivative(node->childs[1]);
                    return res;
                }
                else
                {
                    return derivative(node->childs[1]);
                }
            }
            break;
    }
    printf("ERROR!\n");
    return NULL;
}



int main(int argc, char **argv)
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

    if (argc > 1 && strcmp(argv[1], "t") == 0)
    {
        return 0;
    }


    tree = normalize_tree(tree);
    

    printf("Normalized tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    tree = optimize_tree(tree, 500);
    

    printf("Optimized tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    printf("Exported:\n");
    t = export_basic(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    if (argc > 1 && strcmp(argv[1], "i") == 0)
    {
        printf("Press return to continue\n");
        getchar();
    }


    tree = derivative(tree);
    

    printf("Derivative tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);
    

    printf("Derivative export:\n");
    t = export_basic(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    if (argc > 1 && strcmp(argv[1], "i") == 0)
    {
        printf("Press return to continue\n");
        getchar();
    }


    tree = optimize_tree(tree, 5000);
    

    printf("Optimized derivative tree:\n");
    t = export_ast(buf, tree);
    *t = 0;
    printf("%s\n", buf);


    printf("Exported:\n");
    t = export_basic(buf, tree);
    *t = 0;
    printf("%s\n", buf);

    free(buf);
    
    return 0;
}







