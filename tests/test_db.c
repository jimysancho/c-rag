#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/parser.h"
#include "../src/chunk.h"
#include "../src/db.h"

void test_db_insert(db_t *db) {

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

    for (size_t s = 0; s < chunks.n_chunks; s++) {
        db_insert(db, chunks.chunks[s]);
    }

    chunks_free(chunks);
    free_files(&file, 1);
    return;
}


void test_db_read(db_t *db) {
    char *hash = "14f269c8869106cecb443419060edfec7c17c4292fbe7c3c78b0612411e6ed3f";
    chunk_t *chunk = db_retrieve(db, hash);
    assert(strcmp(chunk->hash, hash) == 0);
    chunk_visualize(chunk);
    free(chunk);
    free(chunk->metadata.path);
    free(chunk->content);
}


void test_db_bulk_retrieve(db_t *db) {
    char *hash[5] = {
        "14f269c8869106cecb443419060edfec7c17c4292fbe7c3c78b0612411e6ed3f",
        "a6410f59433582413aaf1c43ffc846849b4bec39b4d77a67e9c7d168b3d7435a",
        "cd6bc79f02509187926465e19a6ee122dae6a3f3964258ed26aead3f90d47662",
        "43b814d8ee5809f46f51be42ad392994ff17f84fbedc1a65b807f17b54ab455c",
        "aa12d262ec5f63ff71d7d4435b8a5ebd916f0b6b7cc0cc16d4f99085d1939606"
    };
    chunks_t chunks = db_bulk_retrieve(
        db, hash, 5
    );
    assert(chunks.n_chunks == 5);
    for (size_t n = 0; n < chunks.n_chunks; n++) {
        size_t equal = 0;
        chunk_t *chunk = chunks.chunks[n];
        for (size_t k = 0; k < chunks.n_chunks; k++) {
            char *h = hash[k];
            if (strcmp(h, chunk->hash) == 0) equal = 1;
        }
        assert(equal > 0);
    }
}

int main() {
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 1
    };
    // test_db_insert(&db);
    // test_db_read(&db);
    test_db_bulk_retrieve(&db);
    return 0;
}