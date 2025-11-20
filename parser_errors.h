#ifndef PARSER_ERRORS
#define PARSER_ERRORS


#include "stdio.h"
#include "parse.h"
#include "parser_api.h"


#ifdef SEE_VERBOSE_ERRORS
double get_error_record_score(struct parser_error_record_t *record, struct parser_error_table_t *error);
#endif

int print_errors(FILE *file, struct parse_result_t *result, struct parser_error_table_t *error);


#endif
