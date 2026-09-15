#ifndef INGESTION_H
#define INGESTION_H


#include "../chunk.h"
#include "../db.h"


typedef struct __pipeline_ingestion_t {
    char *path;
    chunker_t *chunker;
    size_t n_threads;
    db_t db;
} pipeline_ingestion_t;


typedef struct __pipeline_result_t {
    chunks_t *chunks;
    double time;
} pipeline_result_t;
#endif