#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "ds.h"

#define JSON_BUCKETS 5


void json_visualize(hash_map_t *json, size_t depth) {
    char *tabs = 0;
    if (depth > 0) { 
        tabs = malloc(sizeof(depth) + 1);
        for (size_t i = 0; i < depth; i++) {
            tabs[i] = '\t';
        }
        tabs[depth] = '\0';
    }
    if (depth == 0) {
        printf("====================================\n");
    }
    h_keys_t keys_attr = hash_map_get_keys(json);
    printf("%sJSON VISUALIZATION: %d keys\n", tabs, (int)keys_attr.n_keys);

    ll_t *keys = keys_attr.keys;
    node_t *curr = keys->head;

    while (curr) {
        char *key = (char *)curr->data;
        h_object_t *h_obj = hash_map_get(json, key);
        if (!h_obj) {
            printf("Something went wrong. Could not find key %s\n", key);
            exit(1);
        }
        json_object_t *j_obj = (json_object_t *)h_obj->value;
        switch (j_obj->type) {
            case STRING:
                printf("%sKey: %s. STRING: %s\n", tabs, key, j_obj->as.string);
                break;
            case LIST: {
                ll_t *list = j_obj->as.list->body;
                printf("%sKey: %s. LIST (%zu items)\n", tabs, key, list->size);
                node_t *curr = list->head;
                size_t k = 0;
                while (curr && k < 10) {
                    json_object_t *obj = (json_object_t *)curr->data;
                    if (obj->type == STRING) {
                        printf("%s%s\n", tabs, obj->as.string);
                    } else if (obj->type == OBJECT) {
                        json_visualize(obj->as.json, depth + 1);
                    }
                    curr = curr->next;
                    k++;
                }
                break;
            }
            case OBJECT:
                printf("%sKey: %s. Nested json\n", tabs, key);
                json_visualize(j_obj->as.json, depth + 1);
                break;

            case INTEGER:
            case FLOAT:
                break;
            default:
                break;
        }
        curr = curr->next;
    }
    if (depth == 0) {
        printf("\n====================================\n");
    }
    if (depth > 0) free(tabs);
    return;
}


void skip_whitespace(char *text, int *curr) {
    while (text[*curr] == ' ' || text[*curr] == '\t') { 
        (*curr)++; 
    }
}


char *json_parse_string_value(char *text, int *curr) {
    (*curr)++; // to skip '"'
    skip_whitespace(text, curr);

    size_t start = *curr;
    while (text[*curr] != '"' && text[*curr - 1] != '\'') (*curr)++;
    size_t end = *curr;

    size_t size = end - 1 - start + 1 + 1;
    char *string_value = malloc(size);
    if (!string_value) {
        return NULL;
    }

    memcpy(string_value, &text[start], size);
    string_value[size - 1] = '\0';
    printf("Value extracted: %s\n", string_value);
    (*curr)++;
    printf("Text after value: %s\n", &text[*curr]);
    return string_value;
}


char *json_extract_key(char *text, int *curr) {
    skip_whitespace(text, curr);
    while (text[*curr] != '"') (*curr)++;

    (*curr)++; // next character of ""
    skip_whitespace(text, curr);
    size_t start_key = *curr;

    while (text[*curr] != '"') (*curr)++;
    size_t end_key = *curr;

    size_t size = end_key - 1 - start_key + 1;
    char *key = malloc(size);
    if (!key) {
        return NULL;
    }

    memcpy(key, &text[start_key], size);
    key[size] = '\0';

    printf("Key encountered: %s\n", key);

    // skip '"'
    (*curr)++;

    // now we need to extract the value. the value itself can be another json, a string, etc
    skip_whitespace(text, curr);
    if (text[*curr] != ':') {
        printf("Wrong json\n");
        return NULL;
    }

    (*curr)++;
    // this make sure we are after ":"
    return key;
}


jlist_t *json_parse_list_value(char *text, int *curr) {
    skip_whitespace(text, curr);

    // this is a list. We can have either an object list, or just "items", "content"
    jlist_t *jlist = malloc(sizeof(jlist_t));
    if (!jlist) return NULL;
    jlist->body = malloc(sizeof(ll_t));
    if (!jlist->body) {
        free(jlist);
        return NULL;
    }

    while (text[*curr] != ']') {

        printf("List item: %zu\n", jlist->body->size);

        skip_whitespace(text, curr);

        if (text[*curr] == '{') {
            printf("JSON object inside list: %s\n", &text[*curr]);
            json_object_t *json = malloc(sizeof(json_object_t));
            json->type = OBJECT;
            json->as.json = malloc(sizeof(hash_map_t));
            hash_map_init(json->as.json, 16);
            json_parse_bracket(json->as.json, text, curr);
            (*curr)++;
            printf("JSON object obtained within list -> %s\n", &text[*curr]);
            ll_push(jlist->body, (void *)json);
        } else {
            // any other thing is an array 
            char *text_copy = strdup(&text[*curr]);
            for (char *word = strtok(text_copy, ","); word != NULL; word = strtok(NULL, ",")) {
                char *tmp_word;
                int comma_offset = 0;
                if (strstr(word, "]") != NULL) {
                    tmp_word = malloc(32);
                    if (!tmp_word) return NULL;
                    size_t c = 0;
                    while (word[c] != ']') {
                        if (c >= 32) {
                            tmp_word = realloc(tmp_word, strlen(tmp_word) * 2 + 1);
                        }
                        tmp_word[c] = word[c];
                        c++;
                    }
                    tmp_word[c] = '\0';
                    tmp_word = realloc(tmp_word, strlen(tmp_word) + 1);
                    tmp_word[strlen(tmp_word)] = '\0';
                } else {
                    tmp_word = malloc(strlen(word) + 1);
                    memcpy(tmp_word, word, strlen(word));
                    tmp_word[strlen(word)] = '\0';
                    comma_offset = 1;
                }
                *curr += strlen(tmp_word) + comma_offset; // +1 -> because of the comma
                json_object_t *tmp_obj = malloc(sizeof(json_object_t));
                tmp_obj->type = STRING;
                tmp_obj->as.string = tmp_word;
                ll_push(jlist->body, (void *)tmp_obj);
                if (comma_offset == 0) break;
            }
            free(text_copy);
        }

        if (text[*curr] != ',' && text[*curr] != ']') {
            printf("Wrong list. Expected ','; got: '%c'\n", text[*curr]);
            printf("%s\n", &text[*curr]);
            exit(1);
        } else if (text[*curr] == ',') {
            (*curr)++;
        }
    }

    return jlist;
}


hash_map_t *json_parse_bracket(hash_map_t *json, char *text, int *curr) {

    while (text[*curr] != '}') {
        printf("Current character: %c\n", text[*curr]);
        // if we are here it means that before this we have encountered a '{'
        char *key = json_extract_key(text, curr);
        if (!key) {
            return NULL;
        }
        skip_whitespace(text, curr);
    
        // after this -> we have already skipped ":"
        json_object_t *obj = malloc(sizeof(json_object_t));
        if (!obj) return NULL;
    
        switch (text[*curr]) {
            case '"': {                
                char *string_value = json_parse_string_value(text, curr);
                if (!string_value) {
                    free(obj);
                    return NULL;
                }
                obj->type = STRING;
                obj->as.string = string_value;
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                printf("String value inserted in hash map -> %s\n", string_value);
                printf("Next text: %s\n", &text[*curr]);
                break;
            }
            case '[': {
                (*curr)++;
                jlist_t *list = json_parse_list_value(text, curr);
                obj->type = LIST;
                obj->as.list = list;
                printf("List extracted (%zu): %s\n", list->body->size, &text[*curr]);
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                if (text[*curr] != ']') {
                    printf("JSON parsing went wrong. Expected: ']' -> %s\n", &text[*curr]);
                    json_visualize(json, 0);
                    exit(1);
                }
                (*curr)++;
                break;
            }
            case '{': {
                printf("Nested json -> %s\n", &text[*curr]);
                (*curr)++;
                hash_map_t *nested_hash_map = malloc(sizeof(hash_map_t));
                if (!nested_hash_map) return NULL;
                hash_map_init(nested_hash_map, 16);
    
                json_parse_bracket(nested_hash_map, text, curr);
                printf("Nested object obtained for key: %s\n", key);
                obj->type = OBJECT;
                obj->as.json = nested_hash_map;

                skip_whitespace(text, curr);
                if (text[*curr] != '}') {
                    printf("JSON parsing went wrong. Expected: '}' -> %s\n", &text[*curr]);
                    exit(1);
                }
                (*curr)++;
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                break;
            }
    
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                // integer or float
                break;
            default:
                break;
        }
        skip_whitespace(text, curr);
    }
    printf("Bracket parsed (%d) -> %s\n", *curr, &text[*curr]);
    return json;
}


json_object_t *json_parse(char *text, int *curr) {
    json_object_t *json = malloc(sizeof(json_object_t));
    if (!json) return NULL;
    json->type = OBJECT;
    json->as.json = malloc(sizeof(hash_map_t));
    if (!json->as.json) {
        free(json);
        return NULL;
    }
    hash_map_init(json->as.json, 16);

    printf("Parsing text: %s\n", &text[*curr]);

    while (text[*curr] != '{') (*curr)++;

    json_parse_bracket(json->as.json, text, curr);
    return json;
}

//TODO: malloc - free properly. if something that should not be null is null -> free, etc