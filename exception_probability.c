#include "parser_errors.h"
#include "stdlib.h"
#include "string.h"
#include "parse.h"
#include "stdio.h"


#ifdef SEE_VERBOSE_ERRORS

double get_distance(const char *a, const char *b)
{
    int n = strlen(a);
    int m = strlen(b);
    
    #ifdef USE_LEVINSTEIN
    int **dp = calloc(1, sizeof(*dp) * (n + 1));
    for (int i = 0; i <= n; ++i)
    {
        dp[i] = calloc(1, sizeof(*dp[i]) * (m + 1));
    }
    for (int i = 0; i <= n; ++i)
    {
        dp[i][0] = i;
    }
    for (int j = 0; j <= m; ++j)
    {
        dp[0][j] = j;
    }
    for (int i = 1; i <= n; ++i)
    {
        for (int j = 1; j <= m; ++j)
        {
            dp[i][j] = dp[i - 1][j - 1] + (a[i - 1] != b[j - 1]);
            if (dp[i][j] > dp[i - 1][j] + 1)
            {
                dp[i][j] = dp[i - 1][j] + 1;
            }
            if (dp[i][j] > dp[i][j - 1] + 1)
            {
                dp[i][j] = dp[i][j - 1] + 1;
            }
        }
    }
    return dp[n][m];
    #else
        #error no error distance method selected
    #endif
    
    return 0.0;
}


double get_error_record_score(struct parser_error_record_t *record, struct parser_error_table_t *error)
{
    size_t prediction_length = strlen(record->prediction);
    char *result_code = malloc(record->position - error->source_code + prediction_length + 1);
    const char *source_code = error->source_code;
    memcpy(result_code, error->source_code, record->position - error->source_code);
    memcpy(result_code + (record->position - error->source_code), record->prediction, prediction_length + 1);

    double distance = get_distance(result_code, source_code);

    free(result_code);
    
    return 0.0 - abs(error->max_parsed_position - record->position) - distance;
}
#endif
