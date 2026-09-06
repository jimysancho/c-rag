#include "../src/json.h"
#include <assert.h>
#include <string.h>


// static void test_json_parser() {
//     // char *text = "{"
//     //         "\"model\":\"text-embedding-3-small\","
//     //         "\"input\":\"this is \"these are some tests\"my query to be embedded\""
//     //     "}";

//     char *text = "{"
//                         "\"model\": {\"key1\": \"value1\"},"
//                         "\"data\": [{ \"embedding\": [1, 2, 3, 4]}]"
//                  "}";

//     hash_map_t *h = json_parse(text);
//     assert(h->n_keys == 2);

//     ll_t *keys = (hash_map_get_keys(h)).keys;
//     assert(keys != NULL);

//     node_t *curr = keys->head;
//     while (curr) {
//         char *key = (char *)curr->data;
//         printf("Key: %s\n", key);
//         assert(strcmp(key, "model") == 0 || strcmp(key, "input") == 0);
//         curr = curr->next;
//     }

//     ll_t *values = (hash_map_get_values(h)).values;
//     assert(values != NULL);

//     curr = values->head;
//     while (curr) {
//         char *value = (char *)curr->data;
//         printf("Value: %s\n", value);
//         assert(
//             strcmp(value, "text-embedding-3-small") == 0 ||
//             strcmp(value, "this is \"these are some tests\"my query to be embedded") == 0
//         );
//         curr = curr->next;
//     }
// }


void _test_json_parser() {
    char *text = "{"
                        "\"model\": {\"key1\": \"value1\"},"
                        "\"data\": [{ \"embedding\": [1, 2, 3, 4]}],"
                        "\"key\": { \"key2\": \"value\", \"key3\": { \"key4\": [1, 2, 3, 4, 5]} }"
                 "}";
    int x = 0;
    json_object_t *json = json_parse(text, &x);
    json_visualize(json->as.json, 0);
}

int main() {
    _test_json_parser();
    return 0;
}