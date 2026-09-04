#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "ds.h"

#define JSON_BUCKETS 5


hash_map_t *json_parse(char *text) {
    // NOTE (important) text is a valid json. That's an assumption. I'm not building a validator, just a json parse
    // to easily access keys and values

    printf("%s\n", text);
    hash_map_t *h = malloc(sizeof(hash_map_t));
    if (!h) {
        return NULL;
    }

    hash_map_init(h, JSON_BUCKETS);

    size_t length = strlen(text);

    size_t start_key = 0;
    size_t start_value = 0;

    size_t key_found = 0;
    size_t value_found = 0;

    char *key = NULL;
    char *value = NULL;

    for (size_t c = 0; c < length; c++) {
        if (text[c] == '"' && !key_found) {
            // key has not been found yet
            if (start_key == 0) {
                start_key = c;
            } else {
                // if we are here it means that start has been set
                size_t size = c - 1 - start_key - 1 + 1;
                key = malloc(size);
                memcpy(key, &text[start_key + 1], size);
                key_found = 1;
                start_key = 0;
            }
        } else if (text[c] == '"' && key_found) {

            // we need to get the value
            if (start_value == 0) {
                start_value = c;
            } else {
                if (c < length - 1) {
                    if (text[c + 1] != '}' && text[c + 1] != ',') {
                        // when encountering escpaed characters we need to check if the next char is }
                        continue;
                    }
                }
                // if we are here it means key has been found, and start value as well
                size_t size = c - 1 - start_value - 1 + 1;
                value = malloc(size);
                memcpy(value, &text[start_value + 1], size);
                value_found = 1;
                start_value = 0;
            }
        }

        if (key_found && value_found) {
            key_found = 0;
            value_found = 0;
            hash_map_insert(h, key, (void *)value, sizeof(value));
        }
    }

    return h;

}