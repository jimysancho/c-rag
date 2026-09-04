#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ds.h"


void ll_init(ll_t *ll) {
    ll->head = NULL;
    ll->tail = NULL;
    ll->size = 0;
}


void ll_free(ll_t *ll) {
    node_t *curr = ll->head;
    while (curr) {
        node_t *next = curr->next;
        free(curr->data);
        free(curr);
        curr = next;
    }
    free(ll);
}


node_t *ll_push(ll_t *ll, void *data) {
    node_t *node = malloc(sizeof(node_t));
    if (!node) {
        return NULL;
    }
    node->data = data;
    node->next = NULL;

    if (!ll->head) {
        ll->head = node;
        ll->tail = node;
    } else {
        ll->tail->next = node;
        ll->tail = node;
    }

    ll->size++;
    return node;
}


node_t *ll_peek(ll_t *ll) {
    return ll->head;
}


node_t *ll_pop(ll_t *ll) {
    if (ll->size == 0) {
        return NULL;
    }
    if (ll->head == ll->tail) {
        node_t *n = ll->head;
        ll->head = NULL;
        ll->tail = NULL;
        ll->size--;
        return n;
    }
    node_t *n = ll->head;
    ll->head = n->next;
    ll->size--;
    return n;
}


size_t hash_string(const char *str, size_t n_buckets) {
    size_t hash = 0x100; // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)*str;
        hash *= 1111111111111111111u; // FNV prime
        str++;
    }
    return hash % n_buckets;
}


void hash_map_init(hash_map_t *h, size_t n_buckets) {
    h->n_buckets = n_buckets;
    h->n_keys = 0;
    h->buckets = malloc(sizeof(ll_t *) * n_buckets);
    for (size_t n = 0; n < n_buckets; n++) {
        h->buckets[n] = NULL;
    }
}


void hash_map_free(hash_map_t *h) {
    for (size_t n = 0; n < h->n_buckets; n++) {
        ll_free(h->buckets[n]);
    }
    free(h);
}


h_object_t *hash_map_insert(hash_map_t *h, char *key, void *value, size_t size) {
    size_t bucket = hash_string((const char *)key, h->n_buckets);
    ll_t *ll = h->buckets[bucket];
    if (ll == NULL) {
        ll = malloc(sizeof(ll_t));
        if (!ll) {
            printf("Could not allocate memory for linked list\n");
            exit(1);
        }
        ll_init(ll);
        h->buckets[bucket] = ll;
    }

    node_t *curr = ll->head;
    while (curr) {
        h_object_t  *curr_data = (h_object_t *)curr->data;
        if (strcmp(curr_data->key, key) == 0) {
            printf("Same key already exists in hash map: %s\n", key);
            exit(1);
        }
        curr = curr->next;
    }

    h_object_t *h_object = malloc(sizeof(h_object_t));
    if (!h_object) {
        printf("Could not allocate memory for h_object\n");
        exit(1);
    }
    h_object->key = key;
    h_object->value = value;
    h_object->size = size;

    if (ll_push(ll, (void *)h_object) == NULL) {
        return NULL;
    }
    h->n_keys++;
    return h_object;
}


h_keys_t hash_map_get_keys(hash_map_t *h) {
    ll_t *keys = malloc(sizeof(ll_t));
    ll_init(keys);
    for (size_t n = 0; n < h->n_buckets; n++) {
        ll_t *bucket = h->buckets[n];
        if (bucket == NULL) continue;
        node_t *curr = bucket->head;
        while (curr) {
            h_object_t *curr_data = (h_object_t *)curr->data;
            if (ll_push(keys, curr_data->key) == NULL) {
                exit(1);
            }
            curr = curr->next;
        }
    }
    return (h_keys_t) {
        .n_keys = h->n_keys,
        .keys = keys
    };
}

h_values_t hash_map_get_values(hash_map_t *h) {
    ll_t *values = malloc(sizeof(ll_t));
    ll_init(values);
    for (size_t n = 0; n < h->n_buckets; n++) {
        ll_t *bucket = h->buckets[n];
        if (bucket == NULL) continue;
        node_t *curr = bucket->head;
        while (curr) {
            h_object_t *curr_data = (h_object_t *)curr->data;
            if (ll_push(values, curr_data->value) == NULL) {
                exit(1);
            }
            curr = curr->next;
        }
    }
    return (h_values_t) {
        .n_values = h->n_keys,
        .values = values
    };
}

h_object_t *hash_map_get(hash_map_t *h, char *key) {
    size_t bucket = hash_string((const char *)key, h->n_buckets);
    ll_t *ll = h->buckets[bucket];

    if (ll == NULL) {
        printf("Bucket %zu uninitialized\n", bucket);
        exit(1);
    }

    node_t *curr = ll->head;
    while (curr) {
        h_object_t *curr_data = (h_object_t *)curr->data;
        if (curr_data->key == key) {
            return curr_data;
        }
        curr = curr->next;
    }
    return NULL;
}
