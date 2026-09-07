#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "chunk.h"
#include "openai.h"
#include "ds.h"
#include "json.h"


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

    // chunker_t chunker = (chunker_t) {
    //     .strategy = FIXED_SIZE_CHUNKING,
    //     .params = (chunking_strategy_params_t) {
    //         .strategy = FIXED_SIZE_CHUNKING,
    //         .as = {
    //             (fixed_size_params_t) {
    //                 .size = 10
    //             }
    //         }
    //     }
    // };

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

    chunk_t *chunk = chunks.chunks[0];
    char *response = compute_embedding(chunk->content);
    json_object_t *j = json_parse(response);
    free(response);

    printf("%zu chunks created\n", chunks.n_chunks);

    chunks_visualize(chunks);

    chunks_free(chunks);
    json_object_free(j);
    free_files(&file, 1);
    return 0;
}