#include "parser_errors.h"
#include "parse.h"
#include "inttypes.h"
#include "string.h"
#include "stdio.h"
#include "ctype.h"
#include "math.h"


int print_listing(FILE *file, const char *source, const char *position)
{
    char code_example[80] = {};
    {
        int end = 0;
        char *exmp = code_example;
        for (const char *s = position - 40; s < position + 40; ++s)
        {
            if (s > source && !end)
            {
                end |= (*s == 0);
            }
            if (s < source || end)
            {
                *exmp++ = ' ';
            }
            else
            {
                if (iscntrl(*s) || *s == '\n' || *s == '\v' || *s == '\t')
                {
                    *exmp++ = '\\';
                }
                else
                {
                    *exmp++ = *s;
                }
            }
        }
    }
    fprintf(file, "here: %.*s \n", (int)sizeof(code_example), code_example);
    for (size_t i = 0; i < sizeof(code_example)/sizeof(char); ++i)
    {
        code_example[i] = L' ';
    }
    code_example[40] = 'A';
    fprintf(file, "[here]%.*s \n", (int)sizeof(code_example), code_example);

    return 1;
}


int print_errors(FILE *file, struct parse_result_t *result, struct parser_error_table_t *error)
{
    if (error == NULL)
    {
        return 1;
    }
    
    fprintf(file, "Compilation error:\n");
    fprintf(file, "At symbol %"PRIuPTR":\n", error->max_parsed_position - error->source_code);
    fprintf(file, "Last parsed node is <%d> [see enum node_type_t], parsing was failed\n", error->max_parsed_node_type);
    print_listing(file, error->source_code, error->max_parsed_position);
    #ifdef SEE_VERBOSE_ERRORS  
        remove_duplicates_records(result);

        double *scores = malloc(sizeof(*scores) * result->table_length);
        double max_score = -INFINITY;
        for (size_t v = 0; v < result->table_length; ++v)
        {
            scores[v] = get_error_record_score(result->table + v, error);
            if (scores[v] > max_score)
            {
                max_score = scores[v];
            }
        }
        
        /* print all variants */
        fprintf(file, "Located %"PRIuPTR" possible meanings:\n", result->table_length);
        for (size_t v = 0; v < result->table_length; ++v)
        {
            double score = scores[v];
            fprintf(file, "Variant %"PRIuPTR": at position %"PRIuPTR" may be <%s>? : SCORE %lf\n", 
                          v, 
                          result->table[v].position - error->source_code, 
                          result->table[v].prediction, 
                          score);
            if (score == max_score)
            {
                print_listing(file, error->source_code, result->table[v].position);
            }
            fprintf(file, "\n");
        }
    #endif

    return 0;
}
