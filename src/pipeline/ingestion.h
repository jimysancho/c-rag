#ifndef INGESTION_H
#define INGESTION_H

#include "../chunk.h"
#include "../db.h"


typedef struct __thread_arg {
    chunks_t *chunks;
    size_t index;
} thread_arg;


typedef struct __pipeline_ingestion_t {
    char *path;
    chunker_t *chunker;
    size_t n_threads;
    db_t *db;
    size_t verbose;
} pipeline_ingestion_t;


typedef struct __pipeline_result_t {
    chunks_t chunks;
    chunks_t inserted_chunks;
    float time;
} pipeline_result_t;


pipeline_result_t pipeline_ingestion_run(pipeline_ingestion_t pipeline);
#endif