#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "ds.h"

#define JSON_BUCKETS 5


void jlist_free(jlist_t *jlist) {
    if (!jlist) return;
    ll_t *j_ll = jlist->body;
    if (!j_ll) {
        free(jlist);
        return;
    }
    ll_free(j_ll, 1);
    free(jlist);
}


void json_object_free(json_object_t *j_obj) {
    if (!j_obj) return;
    switch (j_obj->type) {
        case STRING:
            free(j_obj->as.string);
            break;
        case LIST: {
            jlist_t *jlist = j_obj->as.list;
            node_t *l = jlist->body->head;
            for (; l != NULL; l = l->next) {
                json_object_free((json_object_t *)l->data);
            }
            jlist_free(jlist);
            break;
        }
        case OBJECT: {
            h_keys_t keys = hash_map_get_keys(j_obj->as.json);
            ll_t *keys_ll = keys.keys;
            node_t *n = keys_ll->head;
            for (; n != NULL; n = n->next) {
                h_object_t *value = hash_map_get(j_obj->as.json, (char *)n->data);
                if (!value) {
                    printf("Something went wrong when fetching value of key %s\n", (char *)n->data);
                    exit(1);
                }
                json_object_free((json_object_t *)value->value);
            }
            ll_free(keys.keys, 0);
            hash_map_free(j_obj->as.json);
            break;
        }
        case INTEGER:
        case FLOAT:
        default:
            break;
    }
    return;
}


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
            printf("Something went wrong. Could not find key '%s'\n", key);
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
    ll_free(keys, 0);
    return;
}

void skip_whitespace(char *text, int *curr) {
    while (text[*curr] == ' ' || text[*curr] == '\t' || text[*curr] == '\n') { 
        (*curr)++; 
    }
}


char *json_parse_string_value(char *text, int *curr) {
    if (text[*curr] == '"') (*curr)++; // to skip '"'
    skip_whitespace(text, curr);

    size_t start = *curr;
    char keyword[5];
    size_t i = 0;
    while (text[*curr] != '"' || text[*curr - 1] == '\\') {
        if (i <= 3) {
            keyword[i] = text[*curr];
        } else if (i == 4) {
            keyword[i] = '\0';
            size_t found = 0;
            if (strcmp(keyword, "null") == 0) {
                found = 1;
            } else if (strcmp(keyword, "true") == 0) {
                found = 1;
            } else if (strcmp(keyword, "false") == 0) {
                found = 1;
            }

            if (found) {
                char *word = malloc(sizeof(keyword));
                memcpy(word, keyword, sizeof(keyword) + 1);
                word[strlen(keyword)] = '\0';
                return word;
            }
        }
        (*curr)++;
        i++;
    }
    size_t end = *curr;

    size_t size = end - 1 - start + 1 + 1;
    char *string_value = malloc(size);
    if (!string_value) {
        return NULL;
    }

    memcpy(string_value, &text[start], size);
    string_value[size - 1] = '\0';
#ifdef DEBUG
    printf("Value extracted: %s\n", string_value);
#endif
    (*curr)++;
#ifdef DEBUG
    printf("Text after value: %s\n", &text[*curr]);
#endif
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
    char *key = malloc(size + 1); // for null terminator
    if (!key) {
        return NULL;
    }

    memcpy(key, &text[start_key], size);
    key[size] = '\0';

#ifdef DEBUG
    printf("Key encountered: %s\n", key);
#endif

    // skip '"'
    (*curr)++;

    // now we need to extract the value. the value itself can be another json, a string, etc
    skip_whitespace(text, curr);
    if (text[*curr] != ':') {
        printf("Wrong json\n");
        free(key);
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

        #ifdef DEBUG
        printf("List item: %zu\n", jlist->body->size);
        #endif

        skip_whitespace(text, curr);

        if (text[*curr] == '{') {
            #ifdef DEBUG
            printf("JSON object inside list: %s\n", &text[*curr]);
            #endif
            json_object_t *json = malloc(sizeof(json_object_t));

            if (!json) {
                jlist_free(jlist);
                return NULL;
            }

            json->type = OBJECT;
            json->as.json = malloc(sizeof(hash_map_t));

            if (!json->as.json) {
                jlist_free(jlist);
                return NULL;
            }

            hash_map_init(json->as.json, 16);
            json_parse_bracket(json->as.json, text, curr);
            (*curr)++;
            #ifdef DEBUG
            printf("JSON object obtained within list -> %s\n", &text[*curr]);
            #endif

            ll_push(jlist->body, (void *)json);
        } else {
            // any other thing is an array 
            char *text_copy = strdup(&text[*curr]);
            char *saveptr;
            // NOTE: strtok is not thread safe. Therefore, when calling mulitple threads, internal state of strtok can 
            // overlap, causing parsing issues
            for (char *word = strtok_r(text_copy, ",", &saveptr); word != NULL; word = strtok_r(NULL, ",", &saveptr)) {
                char *tmp_word;
                int comma_offset = 0;
                if (strstr(word, "]") != NULL) {
                    tmp_word = malloc(32);
                    if (!tmp_word) {
                        jlist_free(jlist);
                        free(text_copy);
                        return NULL;
                    }
                    size_t c = 0;
                    size_t max_size = 32;
                    while (word[c] != ']') {
                        if (c >= max_size) {
                            max_size *= 2;
                            tmp_word = realloc(tmp_word, strlen(tmp_word) * 2 + 1);
                            if (!tmp_word) {
                                jlist_free(jlist);
                                free(text_copy);
                                return NULL;
                            }
                        }
                        tmp_word[c] = word[c];
                        c++;
                    }
                    tmp_word[c] = '\0';
                    tmp_word = realloc(tmp_word, strlen(tmp_word) + 1);
                    if (!tmp_word) {
                        jlist_free(jlist);
                        free(text_copy);
                        return NULL;
                    }
                    tmp_word[strlen(tmp_word)] = '\0';
                } else {
                    tmp_word = malloc(strlen(word) + 1);
                    if (!tmp_word) {
                        jlist_free(jlist);
                        free(text_copy);
                        return NULL;
                    }
                    memcpy(tmp_word, word, strlen(word));
                    tmp_word[strlen(word)] = '\0';
                    comma_offset = 1;
                }
                *curr += strlen(tmp_word) + comma_offset; // +1 -> because of the comma
                json_object_t *tmp_obj = malloc(sizeof(json_object_t));
                if (!tmp_obj) {
                    jlist_free(jlist);
                    free(text_copy);
                    return NULL;
                }
                tmp_obj->type = STRING;
                tmp_obj->as.string = tmp_word;
                ll_push(jlist->body, (void *)tmp_obj);
                if (comma_offset == 0) break;
            }
            free(text_copy);
        }
        skip_whitespace(text, curr);
        if (text[*curr] != ',' && text[*curr] != ']') {
            printf("Wrong list. Expected ','; got: '%c' (%d)\n", text[*curr], *curr);
            char temp[64];
            memcpy(temp, &text[*curr - 10], 63);
            temp[63] = '\0';
            printf("temp -> %s\n", temp);
            printf("%c\n", text[*curr - 1]);
            exit(1);
        } else if (text[*curr] == ',') {
            (*curr)++;
        }
    }

    return jlist;
}


hash_map_t *json_parse_bracket(hash_map_t *json, char *text, int *curr) {

    while (text[*curr] != '}') {
        #ifdef DEBUG
        printf("Current character: %c\n", text[*curr]);
        #endif

        // if we are here it means that before this we have encountered a '{'
        char *key = json_extract_key(text, curr);
        if (!key) {
            hash_map_free(json);
            return NULL;
        }
        skip_whitespace(text, curr);
    
        // after this -> we have already skipped ":"
        json_object_t *obj = malloc(sizeof(json_object_t));
        if (!obj) {
            free(key);
            free(obj);
            hash_map_free(json);
            return NULL;
        }
    
        switch (text[*curr]) {
            case '[': {
                (*curr)++;
                jlist_t *list = json_parse_list_value(text, curr);
                if (!list) {
                    free(key);
                    free(obj);
                    hash_map_free(json);
                }
                obj->type = LIST;
                obj->as.list = list;
                #ifdef DEBUG
                printf("List extracted (%zu): %s\n", list->body->size, &text[*curr]);
                #endif

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
                #ifdef DEBUG
                printf("Nested json -> %s\n", &text[*curr]);
                #endif

                (*curr)++;
                hash_map_t *nested_hash_map = malloc(sizeof(hash_map_t));
                if (!nested_hash_map) {
                    free(key);
                    free(obj);
                    hash_map_free(json);
                    return NULL;
                }
                hash_map_init(nested_hash_map, 16);
    
                if (!json_parse_bracket(nested_hash_map, text, curr)) {
                    free(key);
                    free(obj);
                    free(json);
                    return NULL;
                }
                #ifdef DEBUG
                printf("Nested object obtained for key: %s\n", key);
                #endif

                // NOTE: the leaks I'm getting I think happens at this point: with nested objects
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
                free(key);
                free(obj);
                (*curr)++;
                break;
            case '"': 
            default: {
                char *string_value = json_parse_string_value(text, curr);
                if (!string_value) {
                    free(key);
                    free(obj);
                    hash_map_free(json);
                    return NULL;
                }
                obj->type = STRING;
                obj->as.string = string_value;
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                #ifdef DEBUG
                printf("String value inserted in hash map -> %s\n", string_value);
                printf("Next text: %s\n", &text[*curr]);
                #endif

                break;
            }
        }
        skip_whitespace(text, curr);
    }
    #ifdef DEBUG
    printf("Bracket parsed (%d) -> %s\n", *curr, &text[*curr]);
    #endif

    return json;
}


json_object_t *json_parse(char *text) {
    json_object_t *json = malloc(sizeof(json_object_t));
    if (!json) return NULL;
    json->type = OBJECT;
    json->as.json = malloc(sizeof(hash_map_t));
    if (!json->as.json) {
        free(json);
        return NULL;
    }
    hash_map_init(json->as.json, 16);

    int curr = 0;

    #ifdef DEBUG
    printf("Parsing text: %s\n", &text[curr]);
    #endif


    while (text[curr] != '{') (curr)++;

    json_parse_bracket(json->as.json, text, &curr);
    return json;
}
