#ifndef INGESTION_H
#define INGESTION_H

#include "../chunk.h"
#include "../db.h"


typedef struct __ing_thread_arg {
    chunks_t *chunks;
    size_t index;
} ing_thread_arg;


typedef struct __pipeline_ingestion_t {
    char *path;
    chunker_t *chunker;
    size_t n_threads;
    db_t *db;
    size_t verbose;
} pipeline_ingestion_t;


typedef struct __ingestion_result_t {
    chunks_t chunks;
    float time;
} ingestion_result_t;


ingestion_result_t pipeline_ingestion_run(pipeline_ingestion_t pipeline);
#endif