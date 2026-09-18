#include "pipeline.h"


void get_embedding_from_json(json_object_t *j, float embedding[1536]) {
    h_object_t *data = hash_map_get(j->as.json, "data");
    if (!data) {
        printf("Could not get data from hash_map\n");
        exit(1);
    }
    json_object_t *j_obj = (json_object_t *)data->value;
    if (!j_obj) {
        printf("Could not get j_obj from data\n");
        exit(1);
    }

    jlist_t *ll_data = j_obj->as.list;
    if (!ll_data) {
        printf("Could not get ll from data\n");
        exit(1);
    }

    node_t *data_value = ll_data->body->head;
    if (!data_value) {
        printf("Could not get data value from list\n");
        exit(1);
    }
    j_obj = (json_object_t *)data_value->data;
    h_object_t *embedding_obj = hash_map_get(j_obj->as.json, "embedding");

    j_obj = (json_object_t *)embedding_obj->value;
    ll_t *embedding_ll = j_obj->as.list->body;
    node_t *curr = embedding_ll->head;
    for (size_t n = 0; n < 1536; n++) {
        j_obj = curr->data;
        float emb = (float)atof(j_obj->as.string);
        embedding[n] = emb;
        curr = curr->next;
    }
}

