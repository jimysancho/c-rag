#ifndef RETRIEVAL_H
#define RETRIEVAL_H

#include "../db.h"
#include "../chunk.h"
#include <pthread.h>


typedef struct __retrieval_pipeline_t {
    char *query;
    db_t *db;
    float sim_th;
    size_t n_threads;
    size_t n_chunks;
} retrieval_pipeline_t;


typedef struct __retrieval_chunk_t {
    chunk_t *chunk;
    float similarity;
} retrieval_chunk_t;


typedef struct __ret_thread_arg {
    char hash[SHA256_HEX_LENGTH + 1];
    float query_emb[1536];
    db_t *db;
    float sim_th;
    pthread_mutex_t *lock;
    retrieval_chunk_t **chunks;
    size_t *index;
} ret_thread_arg;


typedef struct __retrieval_result_t {
    retrieval_chunk_t **chunks;
    size_t n_chunks;
} retrieval_result_t;


retrieval_result_t retrieval_pipeline_run(retrieval_pipeline_t pipeline);
#endif