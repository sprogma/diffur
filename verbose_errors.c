#include "parse.h"
#include "parser_errors.h"
#include "stdlib.h"
#include "string.h"


#ifdef SEE_VERBOSE_ERRORS
static int record_compare(const void *a, const void *b)
{
    const struct parser_error_record_t *ra = a;
    const struct parser_error_record_t *rb = b;
    if (ra->position < rb->position)
    {
        return -1;
    }
    else if (ra->position > rb->position)
    {
        return +1;
    }
    return strcmp(ra->prediction, rb->prediction);
}

int remove_duplicates_records(struct parse_result_t *result)
{
    if (result->table_length > 1)
    {
        qsort(result->table, result->table_length, sizeof(*result->table), record_compare);
        size_t id = 0, i = 0;
        result->table[id++] = result->table[i++];
        while (i < result->table_length)
        {
            if (record_compare(&result->table[i], &result->table[i - 1]) != 0)
            {
                result->table[id++] = result->table[i];
            }
            i++;
        }
        result->table_length = id;
    }
    return 0;
}

int update_all_records(struct parse_result_t *result, const char *string)
{
    size_t len = strlen(string);
    for (size_t i = 0; i < result->table_length; ++i)
    {
        size_t curr = strlen(result->table[i].prediction);
        
        char *s = realloc(result->table[i].prediction, curr + len + 1);
        if (s == NULL)
        {
            fprintf(stderr, "No more memory\n");
        }
        result->table[i].prediction = s;
        memcpy(result->table[i].prediction + curr, string, len + 1);
    }
    return 0;
}

int add_table_record(struct parse_result_t *result, const char *position, const char *prediction)
{
    if (result->table_length >= result->table_alloc)
    {
        result->table_alloc = result->table_alloc * 2 + 1 * !(result->table_alloc);
        // TODO:
        result->table = realloc(result->table, sizeof(*result->table) * result->table_alloc);
    }
    result->table[result->table_length++] = (struct parser_error_record_t){position, strdup(prediction)};    
    return 0;
}

int free_table_records(struct parse_result_t *result)
{
    for (size_t i = 0; i < result->table_length; ++i)
    {
        free(result->table[i].prediction);
    }
    free(result->table);
    return 0;
}

int merge_table_records(struct parse_result_t *a, struct parse_result_t *b)
{
    for (size_t i = 0; i < b->table_length; ++i)
    {
        add_table_record(a, b->table[i].position, b->table[i].prediction);
    }
    remove_duplicates_records(a);
    // free(b->table);
    return 0;
}
#endif
