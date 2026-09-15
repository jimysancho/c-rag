#include <stdio.h>
#include <stdlib.h>
#include "../src/parser.h"
#include "../src/chunk.h"
#include "../src/db.h"


int main() {
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 0
    };

    char *path = "./resources/file.txt";

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
                    .window_size = 10,
                    .overlap = 5
                }
            }
        }
    };

    chunks_t chunks = chunks_create(
        &chunker, file
    );

    printf("%zu chunks created\n", chunks.n_chunks);

    for (size_t s = 0; s < chunks.n_chunks; s++) {
        db_insert(&db, chunks.chunks[s]);
    }

    chunks_free(chunks);
    free_files(&file, 1);
    return 0;
}