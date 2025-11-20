#include "parse.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "errno.h"


int update_max_parsed(struct node_t *node, struct parser_error_table_t *error_record);
int isskipchar(char ch);
struct parse_result_t parse_float(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_identifer(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_scalar(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_addsub_delim(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_addsub_child(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_addsub(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_muldiv_delim(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_muldiv_child(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_muldiv(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_braced_expression(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_fncall_delim(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_function_call(const char *code, struct parser_error_table_t *error_record);
struct parse_result_t parse_expression(const char *code, struct parser_error_table_t *error_record);



int update_max_parsed(struct node_t *node, struct parser_error_table_t *error_record)
{
    if (error_record != NULL)
    {
        if (node->end > error_record->max_parsed_position)
        {
            error_record->max_parsed_position = node->end;
            error_record->max_parsed_node_type = node->type;
        }
    }
    return 0;
}


int isskipchar(char ch)
{
    return isspace(ch) && ch != 0;
}


struct parse_result_t parse_float(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }
    const char *start = code;

    char *end;
    (void)strtod(code, &end);

    if (end == NULL || end == code)
    {
        #ifdef SEE_VERBOSE_ERRORS
        struct parser_error_record_t *records = calloc(1, sizeof(*records));
        records[0].position = code;
        records[0].prediction = strdup("0");
        return (struct parse_result_t){NULL, NULL, 1, 1, records};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }

    
    struct node_t *node = new_node();
    node->type = NODE_TYPE_FLOAT;
    node->start = start;
    node->end = end;

    update_max_parsed(node, error_record);
    
    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){end, node, 0, 0, NULL};
    #else
    return (struct parse_result_t){end, node};
    #endif
}


struct parse_result_t parse_identifer(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }
    const char *start = code;
    while (isalpha(*code)) { code++; }
    
    if (code == start)
    {
        #ifdef SEE_VERBOSE_ERRORS
        struct parser_error_record_t *records = calloc(1, sizeof(*records));
        records[0].position = code;
        records[0].prediction = strdup("x");
        return (struct parse_result_t){NULL, NULL, 1, 1, records};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    
    struct node_t *node = new_node();
    node->type = NODE_TYPE_IDENTIFER;
    node->start = start;
    node->end = code;

    update_max_parsed(node, error_record);


    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){code, node, 0, 0, NULL};
    #else
    return (struct parse_result_t){code, node};
    #endif
}

struct parse_result_t parse_scalar(const char *code, struct parser_error_table_t *error_record)
{
    return parse_xor(code, error_record, 2,
                     parse_identifer,
                     parse_float);
}


struct parse_result_t parse_addsub_delim(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }
    if (*code == '+')
    {
        struct node_t *node = new_node();
        node->type = NODE_TYPE_OP_ADD;
        node->start = code;
        node->end = code + 1;

        update_max_parsed(node, error_record);

        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){code + 1, node, 0, 0, NULL};
        #else
        return (struct parse_result_t){code + 1, node};
        #endif
    }
    if (*code == '-')
    {
        struct node_t *node = new_node();
        node->type = NODE_TYPE_OP_SUB;
        node->start = code;
        node->end = code + 1;

        update_max_parsed(node, error_record);
        
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){code + 1, node, 0, 0, NULL};
        #else
        return (struct parse_result_t){code + 1, node};
        #endif
    }
    #ifdef SEE_VERBOSE_ERRORS
    struct parser_error_record_t *records = calloc(1, sizeof(*records));
    records[0].position = code;
    records[0].prediction = strdup("+");
    return (struct parse_result_t){NULL, NULL, 1, 1, records};
    #else
    return (struct parse_result_t){NULL, NULL};
    #endif
}


struct parse_result_t parse_addsub_child(const char *code, struct parser_error_table_t *error_record)
{
    return parse_first(code, error_record, 4, 
                     parse_muldiv, 
                     parse_braced_expression, 
                     parse_function_call, 
                     parse_scalar);
}

struct parse_result_t parse_addsub(const char *code, struct parser_error_table_t *error_record)
{
    struct parse_result_t result = parse_many(&parse_addsub_child,
                                              &parse_addsub_delim,
                                              code,
                                              error_record);
    if (result.rest == NULL)
    {
        #ifdef SEE_VERBOSE_ERRORS
        // TODO:
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    
    result.node->type = NODE_TYPE_ADDSUB;
    result.node->start = code;
    result.node->end = result.rest;

    update_max_parsed(result.node, error_record);

    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){result.rest, result.node, result.table_length, result.table_alloc, result.table};
    #else
    return (struct parse_result_t){result.rest, result.node};
    #endif
}

struct parse_result_t parse_muldiv_delim(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }
    if (*code == '*')
    {
        struct node_t *node = new_node();
        node->type = NODE_TYPE_OP_MUL;
        node->start = code;
        node->end = code + 1;

        update_max_parsed(node, error_record);

        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){code + 1, node, 0, 0, NULL};
        #else
        return (struct parse_result_t){code + 1, node};
        #endif
    }
    if (*code == '/')
    {
        struct node_t *node = new_node();
        node->type = NODE_TYPE_OP_DIV;
        node->start = code;
        node->end = code + 1;

        update_max_parsed(node, error_record);
        
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){code + 1, node, 0, 0, NULL};
        #else
        return (struct parse_result_t){code + 1, node};
        #endif
    }
    #ifdef SEE_VERBOSE_ERRORS
    struct parser_error_record_t *records = calloc(1, sizeof(*records));
    records[0].position = code;
    records[0].prediction = strdup("*");
    return (struct parse_result_t){NULL, NULL, 1, 1, records};
    #else
    return (struct parse_result_t){NULL, NULL};
    #endif
}


struct parse_result_t parse_muldiv_child(const char *code, struct parser_error_table_t *error_record)
{
    return parse_first(code, error_record, 3, 
                         parse_braced_expression, 
                         parse_function_call, 
                         parse_scalar);
}

struct parse_result_t parse_muldiv(const char *code, struct parser_error_table_t *error_record)
{
    struct parse_result_t result = parse_many(&parse_muldiv_child,
                                              &parse_muldiv_delim,
                                              code,
                                              error_record);
    if (result.rest == NULL)
    {
        #ifdef SEE_VERBOSE_ERRORS
        // TODO:
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    
    result.node->type = NODE_TYPE_MULDIV;
    result.node->start = code;
    result.node->end = result.rest;

    update_max_parsed(result.node, error_record);

    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){result.rest, result.node, result.table_length, result.table_alloc, result.table};
    #else
    return (struct parse_result_t){result.rest, result.node};
    #endif
}

struct parse_result_t parse_braced_expression(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }

    const char *start = code;
    
    if (*code != '(')
    {
        #ifdef SEE_VERBOSE_ERRORS
        struct parser_error_record_t *records = calloc(1, sizeof(*records));
        records[0].position = code;
        records[0].prediction = strdup("()");
        return (struct parse_result_t){NULL, NULL, 1, 1, records};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    code++;

    struct parse_result_t result = parse_expression(code, error_record);
    
    #ifdef SEE_VERBOSE_ERRORS
    update_all_records(&result, ")");
    #endif
    
    if (result.rest == NULL)
    {
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    code = result.rest;

    while (isskipchar(*code)) { code++; }
    if (*code != ')')
    {
        free_node(result.node);
        
        #ifdef SEE_VERBOSE_ERRORS
        add_table_record(&result, code, ")");
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    code++;
    
    struct node_t *node = new_node();

    node->type = NODE_TYPE_BRACES;
    node->start = start;
    node->end = code;
    add_child(node, result.node);
    
    update_max_parsed(node, error_record);

    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){code, node, result.table_length, result.table_alloc, result.table};
    #else
    return (struct parse_result_t){code, node};
    #endif
}



struct parse_result_t parse_fncall_delim(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }
    if (*code == ',')
    {
        struct node_t *node = new_node();
        node->type = NODE_TYPE_OP_COMMA;
        node->start = code;
        node->end = code + 1;

        update_max_parsed(node, error_record);

        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){code + 1, node, 0, 0, NULL};
        #else
        return (struct parse_result_t){code + 1, node};
        #endif
    }
    
    #ifdef SEE_VERBOSE_ERRORS
    struct parser_error_record_t *records = calloc(1, sizeof(*records));
    records[0].position = code;
    records[0].prediction = strdup(",");
    return (struct parse_result_t){NULL, NULL, 1, 1, records};
    #else
    return (struct parse_result_t){NULL, NULL};
    #endif
}



struct parse_result_t parse_function_call(const char *code, struct parser_error_table_t *error_record)
{
    while (isskipchar(*code)) { code++; }

    const char *start = code;

    /* parse fn name */
    
    struct parse_result_t name = parse_identifer(code, error_record);

    #ifdef SEE_VERBOSE_ERRORS
    update_all_records(&name, "()");
    #endif

    if (name.rest == NULL)
    {
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, name.table_length, name.table_alloc, name.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }

    code = name.rest;

    /* skip spaces */
    while (isskipchar(*code)) { code++; }
    
    if (*code != '(')
    {
        #ifdef SEE_VERBOSE_ERRORS
        struct parser_error_record_t *records = calloc(1, sizeof(*records));
        records[0].position = code;
        records[0].prediction = strdup("()");
        return (struct parse_result_t){NULL, NULL, 1, 1, records};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    code++;


    struct parse_result_t result = parse_many_or_one(&parse_expression,
                                              &parse_fncall_delim,
                                              code,
                                              error_record);
    result.node->start = code;
    
    #ifdef SEE_VERBOSE_ERRORS
    update_all_records(&result, ")");
    #endif
    
    if (result.rest == NULL)
    {
        free_node(name.node);
        
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    
    code = result.rest;
    result.node->end = code;

    while (isskipchar(*code)) { code++; }
    
    if (*code != ')')
    {
        free_node(name.node);
        free_node(result.node);
        
        #ifdef SEE_VERBOSE_ERRORS
        add_table_record(&result, code, ")");
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    code++;
    
    struct node_t *node = new_node();

    node->type = NODE_TYPE_FN_CALL;
    node->start = start;
    node->end = code;
    add_child(node, name.node);
    add_child(node, result.node);

    result.node->type = NODE_TYPE_FN_ARGS;
    
    update_max_parsed(node, error_record);

    #ifdef SEE_VERBOSE_ERRORS
    return (struct parse_result_t){code, node, result.table_length, result.table_alloc, result.table};
    #else
    return (struct parse_result_t){code, node};
    #endif
}


struct parse_result_t parse_expression(const char *code, struct parser_error_table_t *error_record)
{
    return parse_first(code, error_record, 5, 
                             parse_addsub, 
                             parse_muldiv, 
                             parse_function_call, 
                             parse_braced_expression, 
                             parse_scalar);
}


struct parse_result_t parse_all(const char *code, struct parser_error_table_t *error_record)
{
    if (error_record)
    {
        error_record->source_code = code;
    }

    struct parse_result_t result = parse_expression(code, error_record);
    if (result.rest == NULL)
    {
        #ifdef SEE_VERBOSE_ERRORS
        return (struct parse_result_t){NULL, NULL, result.table_length, result.table_alloc, result.table};
        #else
        return (struct parse_result_t){NULL, NULL};
        #endif
    }
    
    while (isskipchar(*result.rest)) { result.rest++; }
    
    return result;
}
