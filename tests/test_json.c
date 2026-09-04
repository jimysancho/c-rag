#include "../src/json.h"
#include <assert.h>
#include <string.h>


static void test_json_parser() {
    char *text = "{"
            "\"model\":\"text-embedding-3-small\","
            "\"input\":\"this is \"these are some tests\"my query to be embedded\""
        "}";

    hash_map_t *h = json_parse(text);
    assert(h->n_keys == 2);

    ll_t *keys = (hash_map_get_keys(h)).keys;
    assert(keys != NULL);

    node_t *curr = keys->head;
    while (curr) {
        char *key = (char *)curr->data;
        printf("Key: %s\n", key);
        assert(strcmp(key, "model") == 0 || strcmp(key, "input") == 0);
        curr = curr->next;
    }

    ll_t *values = (hash_map_get_values(h)).values;
    assert(values != NULL);

    curr = values->head;
    while (curr) {
        char *value = (char *)curr->data;
        printf("Value: %s\n", value);
        assert(
            strcmp(value, "text-embedding-3-small") == 0 ||
            strcmp(value, "this is \"these are some tests\"my query to be embedded") == 0
        );
        curr = curr->next;
    }
}

int main() {
    test_json_parser();
    return 0;
}