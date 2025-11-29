#ifndef PARSE_MATH
#define PARSE_MATH

#include "inttypes.h"
#include "stdlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef NO_PARSE_DLL_LIB
    #ifdef PARSE_DLL_BUILD
        #define EXPORT __declspec(dllexport)
    #else
        #define EXPORT __declspec(dllimport)
    #endif
#else
    #define EXPORT
#endif
    

enum node_type_t
{
    NODE_TYPE_NULL,
    NODE_TYPE_ADDSUB,
    NODE_TYPE_OP_ADD,
    NODE_TYPE_OP_SUB,
    NODE_TYPE_MULDIV,
    NODE_TYPE_OP_MUL,
    NODE_TYPE_OP_DIV,
    NODE_TYPE_POW,
    NODE_TYPE_OP_POW,
    NODE_TYPE_BRACES,
    NODE_TYPE_FLOAT,
    NODE_TYPE_IDENTIFER,
    NODE_TYPE_OP_COMMA,
    NODE_TYPE_FN_ARGS,
    NODE_TYPE_FN_CALL,
    NODE_TYPE_OP_PREFIX,
};


#define P_INF +9999

const static int priority[] = {
    // NODE_TYPE_NULL,
    P_INF,
    // NODE_TYPE_ADDSUB,
    +100,
    // NODE_TYPE_OP_ADD,
    P_INF,
    // NODE_TYPE_OP_SUB,
    P_INF,
    // NODE_TYPE_MULDIV,
    +200,
    // NODE_TYPE_OP_MUL,
    P_INF,
    // NODE_TYPE_OP_DIV,
    P_INF,
    // NODE_TYPE_POW,
    +300,
    // NODE_TYPE_OP_POW,
    P_INF,
    // NODE_TYPE_BRACES,
    P_INF,
    // NODE_TYPE_FLOAT,
    P_INF,
    // NODE_TYPE_IDENTIFER,
    P_INF,
    // NODE_TYPE_OP_COMMA,
    P_INF,
    // NODE_TYPE_FN_ARGS,
    P_INF,
    // NODE_TYPE_FN_CALL,
    P_INF,
    // NODE_TYPE_OP_PREFIX,
    +400,
};


struct node_t
{
    enum node_type_t type;
    size_t childs_length;
    size_t childs_alloc;
    struct node_t **childs;
    const char *start;
    const char * end;
};


#ifdef SEE_VERBOSE_ERRORS
struct parser_error_record_t
{
    const char *position;
    char *prediction;
};
#endif


struct parse_result_t
{
    const char *rest;
    struct node_t *node;
    /* used if compilation faled */
#ifdef SEE_VERBOSE_ERRORS
    size_t table_length;
    size_t table_alloc;
    struct parser_error_record_t *table;
#endif
};


struct parser_error_table_t
{
    const char *source_code;
    const char *max_parsed_position;
    enum node_type_t max_parsed_node_type;
};

EXPORT struct parse_result_t parse_all(const char *code, struct parser_error_table_t *error_record);

#ifdef __cplusplus
}
#endif


#endif
