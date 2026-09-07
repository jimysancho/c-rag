#ifndef DS_H
#define DS_H


#include <stdio.h>


typedef struct __node_t {
    void *data;
    struct __node_t *next;
} node_t;


typedef struct __ll_t {
    node_t *head;
    node_t *tail;
    size_t size;
} ll_t;


typedef struct __hash_map_t {
    size_t n_keys;
    size_t n_buckets;
    ll_t **buckets;
} hash_map_t;


typedef struct __h_keys_t {
    ll_t *keys;
    size_t n_keys;
} h_keys_t;


typedef struct __h_values_t {
    ll_t *values;
    size_t n_values; // we assume the values are the same type: value_t or something like that
} h_values_t;


typedef struct __h_object_t {
    char *key;
    void *value;
    size_t size; // size of the value probably
} h_object_t;


void ll_init(ll_t *ll);
void ll_free(ll_t *ll, size_t free_data);
node_t *ll_push(ll_t *ll, void *data);
node_t *ll_peek(ll_t *ll);
node_t *ll_pop(ll_t *ll);

size_t hash_string(const char *str, size_t n_buckets);
void hash_map_init(hash_map_t *h, size_t n_buckets);
void hash_map_free(hash_map_t *h);
h_object_t *hash_map_insert(hash_map_t *h, char *key, void *value, size_t size);
h_keys_t hash_map_get_keys(hash_map_t *h);
h_values_t hash_map_get_values(hash_map_t *h);
h_object_t *hash_map_get(hash_map_t *h, char *key);

#endif