#include <assert.h>
#include <dirent.h>
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
                    .window_size = 5,
                    .overlap = 2
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

    DIR *dir = opendir(db->path);
    assert(dir != NULL);
    closedir(dir);

    char *dir_path = "./.db/068b9b27f53b6271d6e3c5e86ea55c3fa71fd0038f4ee8f793cf4cb05471ed36";
    dir = opendir(dir_path);
    assert(dir != NULL);
    closedir(dir);

    chunks_free(chunks);
    free_files(&file, 1);
    return;
}


void test_db_read(db_t *db) {
    char *hash = "068b9b27f53b6271d6e3c5e86ea55c3fa71fd0038f4ee8f793cf4cb05471ed36";
    chunk_t *chunk = db_retrieve(db, hash);
    assert(strcmp(chunk->hash, hash) == 0);
    chunk_visualize(chunk, HASH);
    free(chunk);
    free(chunk->metadata.path);
    free(chunk->content);
}


void test_db_bulk_retrieve(db_t *db) {
    char *hash[5] = {
        "068b9b27f53b6271d6e3c5e86ea55c3fa71fd0038f4ee8f793cf4cb05471ed36",
        "47fbbdfb355104b07cb24b3a659e5b0c3e2f0cd213cf84b4bf62a8ea391668ee",
        "89045b2b221968470d50170a8bd7e34bd45ffae84aa06ea29e5663962d1b6b04",
        "e9b7328121a2aeb5306885a8730e36962d42244248a9b49e8dbc81217cd6cb7e",
        "9e4b0c81416b607a7e7a9e7698c80751b6a9629ee0c85a3839f9f41a6c2cc59b"
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
    chunks_visualize(chunks, HASH);
}

int main() {
    db_t db = (db_t) {
        .path = "./.db",
        .__created = 0
    };
    test_db_insert(&db);
    test_db_read(&db);
    test_db_bulk_retrieve(&db);
    return 0;
}