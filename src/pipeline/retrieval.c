#include "retrieval.h"
#include "pipeline.h"
#include "../chunk.h"
#include "../db.h"
#include "../ds.h"
#include "../math.h"
#include "../openai.h"
#include <string.h>


retrieval_chunk_t *compute_retrieval_chunk(db_t *db, float query_emb[1536], char *hash, float sim_th) {
    chunk_t *chunk = db_retrieve(db, hash);
    if (!chunk) {
        printf("Could not retrieve chunk %s\n", hash);
        exit(1);
    }
    float sim = compute_similarity(query_emb, chunk->embedding);
    retrieval_chunk_t *ret_chunk = malloc(sizeof(retrieval_chunk_t));
    if (!ret_chunk) {
        chunk_free(chunk);
        exit(1);
    }
    if (sim < sim_th) {
        free(ret_chunk);
        chunk_free(chunk);
        return NULL;
    }
    ret_chunk->chunk = chunk;
    ret_chunk->similarity = sim;
    return ret_chunk;
}


void *load_chunk_embedding_and_add(void *arg) {
    ret_thread_arg *ret_arg = (ret_thread_arg *)arg;
    retrieval_chunk_t *ret_chunk = compute_retrieval_chunk(ret_arg->db, 
                                                           ret_arg->query_emb, 
                                                           ret_arg->hash, 
                                                           ret_arg->sim_th);
    if (!ret_chunk) return NULL;
    pthread_mutex_lock(ret_arg->lock);
    ret_arg->chunks[*(ret_arg->index)] = ret_chunk;
    (*ret_arg->index)++;
    pthread_mutex_unlock(ret_arg->lock);
    return NULL;
}


retrieval_result_t retrieval_pipeline_run(retrieval_pipeline_t pipeline) {

    char *response = compute_embedding(pipeline.query);
    json_object_t *j = json_parse(response);
    float query_emb[1536] = {0};
    get_embedding_from_json(j, query_emb);
    free(response);
    json_object_free(j);
    free(j);

    h_keys_t chunks_hash = hash_map_get_keys(pipeline.db->keys);
    node_t *n = chunks_hash.keys->head;
    size_t index = 0;
    size_t count = 0;

    retrieval_chunk_t **retrieved_chunks = malloc(sizeof(retrieval_chunk_t *) * chunks_hash.n_keys);
    ret_thread_arg **args = malloc(sizeof(ret_thread_arg *) * chunks_hash.n_keys);
    if (!args || !retrieved_chunks) exit(1);

    size_t n_threads = chunks_hash.n_keys; // pipeline.n_threads < chunks_hash.n_keys ? pipeline.n_threads : chunks_hash.n_keys;

    pthread_t threads[n_threads];
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    //FIXME: wrong usage of threads
    for (; n != NULL; n = n->next) {
        ret_thread_arg *arg = malloc(sizeof(ret_thread_arg));
        if (!arg) exit(1);
        arg->chunks = retrieved_chunks;
        memcpy(arg->hash, (char *)(n->data), SHA256_HEX_LENGTH);
        arg->hash[SHA256_HEX_LENGTH] = '\0';
        memcpy(arg->query_emb, query_emb, sizeof(query_emb));
        arg->db = pipeline.db;
        arg->sim_th = pipeline.sim_th;
        arg->lock = &lock;
        arg->index = &count;
        args[index] = arg;
        pthread_create(&threads[index], NULL, load_chunk_embedding_and_add, (void *)arg);
        index++;
    }

    for (size_t i = 0; i < index; i++) {
        pthread_join(threads[i], NULL);
        free(args[i]);
    }

    free(args);
    pthread_mutex_destroy(&lock);

    //TODO: order retrieved_chunks by sim, and then resize 

    return (retrieval_result_t) {
        .chunks = retrieved_chunks,
        .n_chunks = count
    };
}