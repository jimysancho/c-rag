#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>

#include "parser.h"
#include "chunk.h"
#include "openai.h"
#include "ds.h"
#include "json.h"
#include "math.h"
#include "db.h"

#define N_THREADS 10


typedef struct __thread_arg {
    chunks_t *chunks;
    size_t index;
} thread_arg;

void get_embedding_from_json(json_object_t *j, float embedding[1536]);


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


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Error: you must provide the path of the file\n");
        exit(1);
    }
    char *path = argv[1];
    file_t *file = load_file_contents(path);
    if (!file) {
        perror("Something went wrong loading the file\n");
        exit(1);
    }

    chunker_t chunker = (chunker_t) {
        .strategy = SLIDING_WINDOW_CHUNKING,
        .params = (chunking_strategy_params_t) {
            .strategy = SLIDING_WINDOW_CHUNKING,
            .as = {
                .sliding_window_params = {
                    .window_size = 5,
                    .overlap = 2
                }
            }
        }
    };

    chunks_t chunks = chunks_create(
        &chunker, file
    );

    curl_global_init(CURL_GLOBAL_DEFAULT);

    //TODO: create a thread pool from which get tasks or something like that
    size_t n_threads = N_THREADS > chunks.n_chunks ? chunks.n_chunks : N_THREADS;
    pthread_t threads[n_threads];
    thread_arg **args = malloc(sizeof(thread_arg *) * chunks.n_chunks);
    for (size_t index = 0; index < chunks.n_chunks; index++) {
        // NOTE: arg needs to be allocated, otherwise, its address will be the same always
        // and therefore it will become a race condition on the index
        thread_arg *arg = (thread_arg *)malloc(sizeof(thread_arg));
        if (!arg) exit(1);
        arg->chunks = &chunks,
        arg->index = index;
        args[index] = arg;
        pthread_create(&threads[index], NULL, thread_compute_embedding, (void *)arg);
    }

    for (size_t index = 0; index < chunks.n_chunks; index++) {
        pthread_join(threads[index], NULL);
        free(args[index]);
    }
    free(args);
    curl_global_cleanup();

    printf("%zu chunks created\n", chunks.n_chunks);

    chunks_visualize(chunks, FULL);

    // NOTE: this has to be a startup type of operation, to avoid multiple threads trying to create it
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 0
    };

    init_db(&db);

    db_bulk_insert(&db, chunks);

    chunks_free(chunks);
    free_files(&file, 1);
    return 0;
}