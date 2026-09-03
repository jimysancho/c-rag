#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "chunk.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Error: you must provide the path of the file\n");
        exit(1);
    }
    char *path = argv[1];
    file_t *file = load_file_contents(path);
    if (!file) {
        perror("Something went wrong loading the file\n");
        exit(1);
    }

    chunker_t chunker = (chunker_t) {
        .strategy = FIXED_SIZE_CHUNKING,
        .params = (chunking_strategy_params_t) {
            .strategy = FIXED_SIZE_CHUNKING,
            .as = {
                (fixed_size_params_t) {
                    .size = 10
                }
            }
        }
    };

    chunks_t chunks = chunks_create(
        &chunker, file
    );

    printf("%zu chunks created\n", chunks.n_chunks);

    chunks_visualize(chunks);

    chunks_free(chunks);
    free_files(&file, 1);
    return 0;
}