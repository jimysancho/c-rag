#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "ds.h"

#define JSON_BUCKETS 5

hash_map_t *json_parse_stack_based(char *text) {
    ll_t *stack_left = malloc(sizeof(stack_left));
    ll_t *stack_right = malloc(sizeof(stack_right));

    if (!stack_left || !stack_right) {
        return NULL;
    }

    ll_init(stack_left); 
    ll_init(stack_right);

    hash_map_t *h = malloc(sizeof(hash_map_t));
    if (!h) {
        return NULL;
    }

    hash_map_init(h, JSON_BUCKETS);

    // this holds the domain itself
    for (size_t c = 0; c < strlen(text); c++) {
        if (text[c] == '{' || text[c] == '[') {
            ll_push(stack_left, (void *)c);
        } else if (text[c] == '}' || text[c] == ']') {
            ll_push(stack_right, (void *)c);
        }
    }

    // the "outer keys are the ones between the first '{' occurrence and the 2nd one"

    ll_free(stack_left);
    ll_free(stack_right);

    return h;

}


hash_map_t *json_parse_(char *text) {
    // NOTE (important) text is a valid json. That's an assumption. I'm not building a validator, just a json parse
    // to easily access keys and values

    printf("%s\n", text);
    // json_parse_stack_based(text);
    // exit(1);

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
        //TODO: important to handle cases where the value will be a list or a json
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

    size_t size = end - 1 - start + 1;
    char *string_value = malloc(size);
    if (!string_value) {
        return NULL;
    }

    memcpy(string_value, &text[start], size);
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
        exit(1);
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
    if (!jlist->body) return NULL;

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
                    if (c > 32) {
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

    return jlist;
}


void *json_parse_bracket(hash_map_t *json, char *text, int *curr) {

    while (text[*curr] != '}') {
        printf("Current character: %c\n", text[*curr]);
        // if we are here it means that before this we have encountered a '{'
        char *key = json_extract_key(text, curr);
        skip_whitespace(text, curr);
    
        // after this -> we have already skipped ":"
        json_object_t *obj = malloc(sizeof(json_object_t));
        if (!obj) return NULL;
    
        switch (text[*curr]) {
            case '"': {                
                char *string_value = json_parse_string_value(text, curr);
                obj->type = STRING;
                obj->as.string = string_value;
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                printf("String value inserted in hash map -> %s\n", string_value);
                (*curr)++;
                break;
            }
            case '[': {
                (*curr)++;
                jlist_t *list = json_parse_list_value(text, curr);
                obj->type = LIST;
                obj->as.list = list;
                printf("List extracted (%zu): %s\n", list->body->size, &text[*curr]);
                node_t *n = list->body->head;
                while (n) {
                    json_object_t *n_j = (json_object_t *)n->data;
                    if (n_j->type == STRING) {
                        printf("%s\n", n_j->as.string);
                    } else {
                        json_visualize(n_j->as.json, 0);
                    }
                    n = n->next;
                }
                hash_map_insert(json, key, (void *)obj, sizeof(obj));
                if (text[*curr] != ']') {
                    printf("JSON parsing went wrong. Expected: ']' -> %s\n", &text[*curr]);
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
                return NULL;
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
    }
    printf("Bracket parsed (%d) -> %s\n", *curr, &text[*curr]);
    return NULL;
}


json_object_t *json_parse(char *text, int *curr) {
    json_object_t *json = malloc(sizeof(json_object_t));
    if (!json) return NULL;
    json->type = OBJECT;
    json->as.json = malloc(sizeof(hash_map_t));
    if (!json->as.json) return NULL;
    hash_map_init(json->as.json, 16);

    printf("Parsing text: %s\n", &text[*curr]);

    while (text[*curr] != '{') (*curr)++;

    json_parse_bracket(json->as.json, text, curr);
    return json;

    while (*curr < (int)strlen(text)) {
        if (text[*curr] == '{') {
            printf("JSON object -> %s\n", &text[*curr]);
            json_parse_bracket(json->as.json, text, curr);
            if (text[*curr] != '}') {
                printf("JSON parsing went wrong. Expected: '}' -> %s\n", &text[*curr]);
                exit(1);
            }
            (*curr)++;
            printf("JSON object finished -> %s\n", &text[*curr]);
        } else if (text[*curr] == ',') {
            printf("Next key will start: %s\n", &text[*curr]);
            // if we encounter a comma at this point we need to extract the key
            // and then whatever is after that
            char *key = json_extract_key(text, curr);
            json_object_t *obj = malloc(sizeof(json_object_t));
            if (!obj) return NULL;

            skip_whitespace(text, curr);
            if (text[*curr] == '[') {
                printf("Extracting LIST object -> %s\n", &text[*curr]);
                (*curr)++;
                jlist_t *list = json_parse_list_value(text, curr);
                printf("LIST object extracted -> %s\n", &text[*curr]);
                obj->type = LIST;
                obj->as.list = list;
                if (text[*curr] != ']') {
                    printf("JSON parsing went wrong. Expected ']' -> %s\n", &text[*curr]);
                    json_visualize(json->as.json, 0);
                    exit(1);
                }
                (*curr)++;
            } else if (text[*curr] == '{') {
                (*curr)++;
                json_object_t *j_obj = json_parse_bracket(json->as.json, text, curr);
                obj->type = OBJECT;
                obj->as.json = j_obj->as.json;
                if (text[*curr] != '}') {
                    printf("JSON parsing went wrong. Expected: '}' -> %s\n", &text[*curr]);
                    json_visualize(json->as.json, 0);
                    exit(1);
                }
                (*curr)++;
            } else {
                printf("Invalid sequence: %c\n", text[*curr]);
                exit(1);
            }
            hash_map_insert(json->as.json, key, (void *)obj, sizeof(obj));
        }
    }

    return json;
}
