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
    return 0;
}
