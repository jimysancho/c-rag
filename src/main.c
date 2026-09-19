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
#include "pipeline/ingestion.h"
#include "pipeline/retrieval.h"

#define N_THREADS 10


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Error: you must provide the path of the file\n");
        exit(1);
    }
    char *path = argv[1];

    // NOTE: this has to be a startup type of operation, to avoid multiple threads trying to create it
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 0
    };

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

    init_db(&db);
    
    pipeline_ingestion_t ingestion_pipeline = (pipeline_ingestion_t) {
        .chunker = &chunker,
        .db = &db,
        .n_threads = N_THREADS,
        .path = path,
        .verbose = 0
    };

    pipeline_result_t ingestion_result = pipeline_ingestion_run(ingestion_pipeline);

    retrieval_pipeline_t ret_pipeline = (retrieval_pipeline_t) {
        .query = "and being able to create",
        .db = &db,
        .sim_th = 0.85,
        .n_threads = ingestion_result.chunks.n_chunks,
        .n_chunks = 4
    };
    retrieval_result_t retrieval_result = retrieval_pipeline_run(ret_pipeline);

    printf("%zu retrieved chunks\n", retrieval_result.n_chunks);
    for (size_t r = 0; r < retrieval_result.n_chunks; r++) {
        retrieval_chunk_t *ret_chunk = retrieval_result.chunks[r];
        printf("sim: %f -> %s\n", ret_chunk->similarity, ret_chunk->chunk->hash);
    }

    chunks_free(ingestion_result.chunks);
    return 0;
}