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
    
    pipeline_ingestion_t pipeline = (pipeline_ingestion_t) {
        .chunker = &chunker,
        .db = &db,
        .n_threads = N_THREADS,
        .path = path,
        .verbose = 0
    };

    pipeline_result_t ingestion_result = pipeline_ingestion_run(pipeline);

    chunks_free(ingestion_result.chunks);
    chunks_free(ingestion_result.inserted_chunks);
    return 0;
}