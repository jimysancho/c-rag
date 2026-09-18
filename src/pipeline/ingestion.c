#include <curl/curl.h>
#include <pthread.h>

#include "ingestion.h"
#include "../json.h"
#include "../ds.h"
#include "../openai.h"



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


void *thread_compute_embedding(void *arg) {
    thread_arg *t_arg = (thread_arg *)arg;

    char *response = compute_embedding(t_arg->chunks->chunks[t_arg->index]->content);
    json_object_t *j = json_parse(response);
    float embedding[1536] = {0};
    get_embedding_from_json(j, embedding);
    chunk_t *chunk = t_arg->chunks->chunks[t_arg->index];
    for (size_t s = 0; s < 1536; s++) {
        chunk->embedding[s] = embedding[s];
    }
    free(response);
    json_object_free(j);
    free(j);
    return NULL;
}


pipeline_result_t pipeline_ingestion_run(pipeline_ingestion_t pipeline) {
    file_t *file = load_file_contents(pipeline.path);
    if (!file) {
        perror("Something went wrong loading the file\n");
        exit(1);
    }

    chunks_t chunks = chunks_create(
        pipeline.chunker, file
    );

    chunks_t chunks_to_insert = (chunks_t) {
        .chunks = malloc(sizeof(chunk_t) * chunks.n_chunks),
        .n_chunks = 0
    };

    if (!chunks_to_insert.chunks) exit(1);
    for (size_t c = 0; c < chunks.n_chunks; c++) {
        chunk_t *chunk = db_retrieve(pipeline.db, chunks.chunks[c]->hash);
        if (chunk != NULL) {
            for (size_t s = 0; s < 1536; s++) {
                chunks.chunks[c]->embedding[s] = chunk->embedding[s];
            }
            chunk_free(chunk);
            continue;
        }
        chunks_to_insert.chunks[chunks_to_insert.n_chunks++] = chunks.chunks[c];
    }

    if (!chunks_to_insert.chunks) {
        free(chunks_to_insert.chunks);
    } else {
        chunks_to_insert.chunks = realloc(chunks_to_insert.chunks, 
                                          sizeof(chunk_t *) * chunks_to_insert.n_chunks);
        if (!chunks_to_insert.chunks) exit(1);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    //TODO: create a thread pool from which get tasks or something like that
    size_t n_threads = pipeline.n_threads > chunks_to_insert.n_chunks ? chunks_to_insert.n_chunks : pipeline.n_threads;
    pthread_t threads[n_threads];
    thread_arg **args = malloc(sizeof(thread_arg *) * chunks.n_chunks);
    for (size_t index = 0; index < chunks_to_insert.n_chunks; index++) {
        // NOTE: arg needs to be allocated, otherwise, its address will be the same always
        // and therefore it will become a race condition on the index
        thread_arg *arg = (thread_arg *)malloc(sizeof(thread_arg));
        if (!arg) exit(1);
        arg->chunks = &chunks_to_insert,
        arg->index = index;
        args[index] = arg;
        pthread_create(&threads[index], NULL, thread_compute_embedding, (void *)arg);
    }

    for (size_t index = 0; index < chunks_to_insert.n_chunks; index++) {
        pthread_join(threads[index], NULL);
        free(args[index]);
    }
    free(args);
    curl_global_cleanup();

    if (pipeline.verbose) {
        printf("%zu chunks created\n", chunks.n_chunks);
        chunks_visualize(chunks, FULL);
    }


    // NOTE: this has to be a startup type of operation, to avoid multiple threads trying to create it
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 0
    };

    init_db(&db);

    db_bulk_insert(&db, chunks);
    free_files(&file, 1);
    return (pipeline_result_t) {
        .chunks = chunks,
        .inserted_chunks = chunks_to_insert,
        .time = 0
    };
}