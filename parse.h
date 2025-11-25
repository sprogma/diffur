#ifndef PARSE_H
#define PARSE_H

#include "inttypes.h"

#include "parser_api.h"

typedef struct parse_result_t (*parser_fn_t)(const char *code, struct parser_error_table_t *error_record);

struct node_t *new_node();
struct node_t *new_node_ex(enum node_type_t type,
                           char *start,
                           char *end);
void free_node(struct node_t *node);
void add_child(struct node_t *node, struct node_t *child);
struct node_t *remove_child(struct node_t *node, size_t child_id);
struct node_t * deep_copy(const struct node_t *node);
struct node_t * soft_copy(const struct node_t *node);

struct parse_result_t parse_many_or_one(parser_fn_t parse_child, parser_fn_t delim, const char *input, struct parser_error_table_t *error_record);
struct parse_result_t parse_many(parser_fn_t parse_child, parser_fn_t delim, const char *input, struct parser_error_table_t *error_record);
struct parse_result_t parse_xor(const char *input, struct parser_error_table_t *error_record, int parsers_count, ...);
struct parse_result_t parse_first(const char *input, struct parser_error_table_t *error_record, int parsers_count, ...);

#ifdef SEE_VERBOSE_ERRORS
int remove_duplicates_records(struct parse_result_t *result);
int add_table_record(struct parse_result_t *result, const char *position, const char *prediction);
int update_all_records(struct parse_result_t *result, const char *string);
int merge_table_records(struct parse_result_t *a, struct parse_result_t *b);
int free_table_records(struct parse_result_t *result);
#endif

#endif
