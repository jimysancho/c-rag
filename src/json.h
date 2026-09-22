#ifndef JSON_H
#define JSON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ds.h"


typedef enum {
    L_OBJ,
    R_OBJ,
    L_QUOTES,
    R_QUOTES,
    L_LIST,
    R_RIGHT,
    SEMICOLUMN
} JSON_TOKEN_TYPE;


typedef struct __json_token_t {
    JSON_TOKEN_TYPE token;
    char *value;
} json_token_t;


typedef enum {
    OBJECT,
    LIST,
    STRING,
    INTEGER,
    FLOAT,
    BOOLEAN
} JSON_OBJECT_TYPE;


typedef struct __jlist_t {
    ll_t *body;
} jlist_t;


typedef struct __json_object_t {
    JSON_OBJECT_TYPE type;
    union {
        hash_map_t *json;
        jlist_t *list;
        size_t bool;
        char *string;
        int integer;
        float flt;
    } as;
} json_object_t;


void skip_whitespace(char *text, int *curr);
char *json_extract_key(char *text, int *curr);

char *json_parse_string_value(char *text, int *curr);
char *json_parse_number_value(char *text, int *curr, int *is_float);
jlist_t *json_parse_list_value(char *text, int *curr);
hash_map_t *json_parse_bracket(hash_map_t *json, char *text, int *curr);
json_object_t *json_parse(char *text);
void json_visualize(hash_map_t *json, size_t depth);

void json_object_free(json_object_t *j_obj);
void jlist_free(jlist_t *jlist);

#endif