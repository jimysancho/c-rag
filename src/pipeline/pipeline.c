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


char *get_content_from_response_dict(char *response) {
    json_object_t *j = json_parse(response);
    h_object_t *obj = hash_map_get(j->as.json, "choices");
    if (!obj) {
        printf("No content in json\n");
        json_visualize(j->as.json, 0);
        return NULL;
    }
    json_object_t *list_obj = (json_object_t *)obj->value;
    jlist_t *list = list_obj->as.list;
    node_t *node = list->body->head;
    json_object_t *body = (json_object_t *)(node->data);
    h_object_t *message_obj = hash_map_get(body->as.json, "message");
    if (!message_obj) {
        printf("No message in json\n");
        json_visualize(j->as.json, 0);
        return NULL;
    }
    json_object_t *message = (json_object_t *)message_obj->value;
    h_object_t *content = hash_map_get(message->as.json, "content");
    if (!content) {
        printf("No content in json\n");
        json_visualize(j->as.json, 0);
        return NULL;
    }

    json_object_t *content_obj = (json_object_t *)content->value;
    char *content_str = strdup(content_obj->as.string);
    json_object_free(j);
    return content_str;
}


char *merge_chunks_content(retrieval_chunk_t **chunks, size_t size, char *query) {
    size_t total_size = 0;
    for (size_t c = 0; c < size; c++) {
        total_size += chunks[c]->chunk->metadata.bytes;
    }
    total_size += size + strlen(query); // size - 1 new lines, and 1 final null character

    char *merged_content = malloc(total_size);
    merged_content[total_size - 1] = '\0';

    size_t offset = 0;

    for (size_t c = 0; c < size; c++) {
        memcpy(merged_content + offset, chunks[c]->chunk->content, chunks[c]->chunk->metadata.bytes);
        memcpy(merged_content + offset + chunks[c]->chunk->metadata.bytes, "\n", 1);
        offset += chunks[c]->chunk->metadata.bytes;
    }
    memcpy(
        merged_content + offset,
        query,
        strlen(query)
    );

    offset += strlen(query);
    merged_content[offset] = '\0';

    return merged_content;
}


pipeline_result_t pipeline_run(pipeline_t pipeline) {
    // basic rag: get relevant chunks using sim - search -> feed them to an llm in some formatted way
    retrieval_result_t retrieval = retrieval_pipeline_run(*pipeline.retrieval_pipeline);
    // now we have N chunks
    char *user_prompt = NULL;
    if (retrieval.n_chunks) {
        user_prompt = merge_chunks_content(retrieval.chunks, retrieval.n_chunks, pipeline.retrieval_pipeline->query);
    } else {
        user_prompt = "No retrieved data\n";
    }
    printf("User prompt: \n%s\n", user_prompt);
    pipeline.generation_pipeline->query = user_prompt;
    generation_result_t generation = generation_pipeline_run(*pipeline.generation_pipeline);

    if (retrieval.n_chunks) free(user_prompt);
    return (pipeline_result_t) {
        .gen_result = generation,
        .ret_result = retrieval
    };
}


void pipeline_clean(pipeline_result_t result) {
    free(result.gen_result.response);
    for (size_t c = 0; c < result.ret_result.n_chunks; c++) {
        chunk_free(result.ret_result.chunks[c]->chunk);
    }
}