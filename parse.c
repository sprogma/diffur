#include "parse.h"
#include "inttypes.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
#include "stdio.h"


struct node_t *new_node()
{
    struct node_t *node = calloc(1, sizeof(*node));

    if (node == NULL) { puts("No more memory"); exit(2); }
    
    return node;
}

struct node_t *new_node_ex(enum node_type_t type,
                           char *start,
                           char *end)
{
    struct node_t *node = calloc(1, sizeof(*node));

    node->type = type;
    node->start = start;
    node->end = end;

    if (node == NULL) { puts("No more memory"); exit(2); }
    
    return node;
}


void free_node(struct node_t *node)
{
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        if (node->childs[i])
        {
            free_node(node->childs[i]);
        }
    }
    free(node);
}


void add_child(struct node_t *node, struct node_t *new_child)
{
    if (node->childs_length >= node->childs_alloc)
    {
        node->childs_alloc = 2 * node->childs_alloc + (!node->childs_alloc);
        struct node_t **new_ptr = realloc(node->childs, sizeof(*node->childs) * node->childs_alloc);
        if (new_ptr == NULL)
        {
            puts("No more memory");
            exit(2);
        }
        node->childs = new_ptr;
    }
    node->childs[node->childs_length++] = new_child;
}


struct node_t * remove_child(struct node_t *node, size_t child_id)
{
    struct node_t *res = node->childs[child_id];
    memmove(node->childs + child_id, node->childs + child_id + 1, sizeof(*node->childs) * (node->childs_length - child_id - 1));
    node->childs_length--;
    return res;
}


struct node_t * deep_copy(struct node_t *node)
{
    struct node_t *res = new_node();
    res->type = node->type;
    res->start = node->start;
    res->end = node->end;
    for (size_t i = 0; i < node->childs_length; ++i)
    {
        add_child(res, deep_copy(node->childs[i]));
    }
    return res;
}



struct parse_result_t parse_many(
    parser_fn_t parse_child,
    parser_fn_t delim,
    const char *input,
    struct parser_error_table_t *error_record
)
{
    #ifdef SEE_VERBOSE_ERRORS
    struct parse_result_t result = {NULL, NULL, 0, 0, NULL};
    #else
    struct parse_result_t result = {NULL, NULL};
    #endif
    
    struct node_t *node = new_node();
    result.node = node;

    struct parse_result_t left = parse_child(input, error_record);
    if (left.rest == NULL) 
    { 
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, left.table_length, left.table_alloc, left.table}; 
        #else
        return (struct parse_result_t){NULL, NULL}; 
        #endif
    }

    add_child(node, left.node);
    #ifdef SEE_VERBOSE_ERRORS
    merge_table_records(&result, &left);
    #endif

    const char *rest = left.rest;
    uint32_t parsed = 0;
    while (1)
    {
        struct parse_result_t mid = delim(rest, error_record);
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &mid);
        #endif
        if (mid.rest == NULL)
        {
            break;
        }
        struct parse_result_t right = parse_child(mid.rest, error_record);
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &right);
        #endif
        if (right.rest == NULL)
        {
            break;
        }
        rest = right.rest;
        add_child(node, mid.node);
        add_child(node, right.node);
        
        parsed = 1;
    }

    if (!parsed)
    {
        free_node(node);
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }

    result.rest = rest;
    return result;
}



struct parse_result_t parse_many_or_one(
    parser_fn_t parse_child,
    parser_fn_t delim,
    const char *input,
    struct parser_error_table_t *error_record
)
{
    #ifdef SEE_VERBOSE_ERRORS
    struct parse_result_t result = {NULL, NULL, 0, 0, NULL};
    #else
    struct parse_result_t result = {NULL, NULL};
    #endif
    
    struct node_t *node = new_node();
    result.node = node;

    struct parse_result_t left = parse_child(input, error_record);
    if (left.rest == NULL) 
    { 
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, left.table_length, left.table_alloc, left.table}; 
        #else
        return (struct parse_result_t){NULL, NULL}; 
        #endif
    }

    add_child(node, left.node);
    #ifdef SEE_VERBOSE_ERRORS
    merge_table_records(&result, &left);
    #endif

    const char *rest = left.rest;
    
    while (1)
    {
        struct parse_result_t mid = delim(rest, error_record);
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &mid);
        #endif
        if (mid.rest == NULL)
        {
            break;
        }
        struct parse_result_t right = parse_child(mid.rest, error_record);
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &right);
        #endif
        if (right.rest == NULL)
        {
            break;
        }
        rest = right.rest;
        add_child(node, mid.node);
        add_child(node, right.node);
    }

    result.rest = rest;
    return result;
}



struct parse_result_t parse_xor(
                const char *input,
                struct parser_error_table_t *error_record,
                int parsers_count,
                ...)
{
    va_list fn_args;
    va_start(fn_args, parsers_count);

    #ifdef SEE_VERBOSE_ERRORS
    struct parse_result_t result = {NULL, NULL, 0, 0, NULL};
    #else
    struct parse_result_t result = {NULL, NULL};
    #endif

    for (int i = 0; i < parsers_count; ++i)
    {
        parser_fn_t fn = va_arg(fn_args, void *);
        
        struct parse_result_t variant = fn(input, error_record);
        
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &variant);
        #endif
        
        if (variant.rest != NULL) 
        {
            if (result.node != NULL)
            {
                fprintf(stderr, "WARNING: not determenistic parsing found. at %s between %d and %d\n", input, variant.node->type, result.node->type);
                free(result.node);
            }
            result.rest = variant.rest;
            result.node = variant.node;
        }
    }

    va_end(fn_args);
    
    return result;
}



struct parse_result_t parse_first(
                const char *input,
                struct parser_error_table_t *error_record,
                int parsers_count,
                ...)
{
    va_list fn_args;
    va_start(fn_args, parsers_count);

    #ifdef SEE_VERBOSE_ERRORS
    struct parse_result_t result = {NULL, NULL, 0, 0, NULL};
    #else
    struct parse_result_t result = {NULL, NULL};
    #endif

    for (int i = 0; i < parsers_count; ++i)
    {
        parser_fn_t fn = va_arg(fn_args, void *);
        
        struct parse_result_t variant = fn(input, error_record);
        
        #ifdef SEE_VERBOSE_ERRORS
        merge_table_records(&result, &variant);
        #endif
        
        if (variant.rest != NULL) 
        {
            va_end(fn_args);
            result.node = variant.node;
            result.rest = variant.rest;
            return result;
        }
    }

    va_end(fn_args);
    
    return result;
}
