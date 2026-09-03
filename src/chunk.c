#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>


#include "chunk.h"


#define CAPACITY 32


void chunk_init(chunk_t *chunk) {
    uuid_generate_random(chunk->chunk_id);
    chunk->hash[0] = '\0';
    chunk->content = NULL;
    memset(chunk->embedding, 0, sizeof(chunk->embedding));
    chunk->children = (chunks_t) {
        .chunks = NULL,
        .n_chunks = 0
    };
    chunk->next = NULL;
    chunk->prev = NULL;
    chunk->parent = NULL;
    chunk->metadata = (chunk_metadata_t) {
        .bytes = 0,
        .path = NULL,
        .strategy = 0
    };
}


void chunk_free(chunk_t *chunk) {
    free(chunk->content);
    // if (chunk->embedding) {
    //     //NOTE -> not sure about this tbh
    //     free(chunk->embedding);
    // }
    free(chunk);
}


void chunks_free(chunks_t chunks) {
    for (size_t n = 0; n < chunks.n_chunks; n++) {
        chunk_free(chunks.chunks[n]);
    }
    free(chunks.chunks);
}


void chunk_visualize(chunk_t *chunk) {
    char uuid_str[37];

    uuid_unparse_lower(chunk->chunk_id, uuid_str);

    printf("Chunk '%s'\n", uuid_str);
    printf("\t- hash: %s\n", chunk->hash);
    printf("\t- content:\n\n%s\n\n", chunk->content);

    if (chunk->prev) {
        char prev_uuid[37];
        uuid_unparse_lower(chunk->prev->chunk_id, prev_uuid);
        printf("\t- prev: %s\n", prev_uuid);
    } else {
        printf("\t- prev: (null)\n");
    }

    if (chunk->next) {
        char next_uuid[37];
        uuid_unparse_lower(chunk->next->chunk_id, next_uuid);
        printf("\t- next: %s\n", next_uuid);
    } else {
        printf("\t- next: (null)\n");
    }

    if (chunk->parent) {
        char parent_uuid[37];
        uuid_unparse_lower(chunk->parent->chunk_id, parent_uuid);
        printf("\t- parent: %s\n", parent_uuid);
    }

    if (chunk->children.chunks) {
        printf("\t- children:\n");

        for (size_t i = 0; i < chunk->children.n_chunks; i++) {
            char child_uuid[37];
            uuid_unparse(chunk->children.chunks[i]->chunk_id, child_uuid);

            printf("\t\t- child%zu: %s\n", i, child_uuid);
        }
    }
}


void chunks_visualize(chunks_t chunks) {
    printf("=====================================\n");
    for (size_t n = 0; n < chunks.n_chunks; n++) {
        chunk_visualize(chunks.chunks[n]);
        printf("=====================================\n");
    }
}


void chunk_compute_hash(chunk_t *chunk) {
    unsigned char digest[SHA256_DIGEST_LENGTH];

    SHA256(
        (unsigned char *)chunk->content,
        strlen(chunk->content),
        digest
    );

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&chunk->hash[i * 2], "%02x", digest[i]);
    }

    chunk->hash[64] = '\0';
}


chunks_t _chunks_create_fixed_size_strategy(chunker_t *chunker, file_t *file) {
    if (chunker->strategy != FIXED_SIZE_CHUNKING) {
        perror("Wrong strategy\n");
        exit(1);
    }
    chunk_t **chunks = malloc(sizeof(chunk_t *) * CAPACITY);

    if (!chunks) {
        printf("Initial allocation for chunks failed\n"); 
        exit(1);
    }

    size_t n_chunks = 0;
    size_t n_words = 0;

    size_t start = 0;
    size_t current_capacity = CAPACITY;

    for (size_t i = 0; i < (size_t)file->bytes; i++) {
        char c = file->contents[i];
        if (c == ' ' || c == '\n' || c == '\t') {
            // we found a space, we increment the number of words seen
            // so far and check wheter or not we have reached the chunk size
            n_words++;
            if (i > 0 && n_words % chunker->params.as.fixed_size_params.size == 0) {
                /*
                size = 2
                hello how are you
                ^ start
                     ^ end (right at the space)
                chunk_size = end - start

                hello how are you
                         ^ start (right at the space, so we would need to increase it +1)
                                ^
                */
                char *chunk_contents = malloc(i - start + 1); // +1 for the null terminator
                if (!chunk_contents) {
                    printf("Could not allocate space for chunk_contents\n");
                    exit(1);
                }
                memcpy(chunk_contents, &file->contents[start], i - start);
                chunk_contents[i - start] = '\0';

                if (n_chunks >= current_capacity) {
                    current_capacity *= 2;
                    chunks = realloc(chunks, current_capacity);
                    if (!chunks) {
                        printf("Reallocation of chunks failed\n");
                        exit(1);
                    }
                }

                chunk_t *chunk = malloc(sizeof(chunk_t));
                if (!chunk) {
                    printf("Could not allocate memory for chunk %zu\n", n_chunks);
                    exit(1);
                }

                chunks[n_chunks] = chunk;
                chunk_init(chunks[n_chunks]);
                chunk->content = chunk_contents;
                chunk_compute_hash(chunk);

                if (n_chunks > 0) {
                    chunk->prev = chunks[n_chunks - 1];
                    chunks[n_chunks - 1]->next = chunk;
                }

                chunk->metadata = (chunk_metadata_t) {
                    .bytes = strlen(chunk_contents),
                    .path = file->path,
                    .strategy = chunker->strategy,
                    .start = start,
                    .end = i
                };

                // we reset n_words for the next chunk
                n_words = 0;
                start = i + 1; // the next start is i + 1 -> next letter jumping the space
                n_chunks++;
                continue;
            }
        }
    }
    return (chunks_t) { .chunks = chunks, .n_chunks = n_chunks };
}


chunks_t chunks_create(chunker_t *chunker, file_t *file) {
    switch (chunker->strategy) {
        case FIXED_SIZE_CHUNKING:
            printf("Fixed size chunking strategy for file: %s\n", file->path);
            return _chunks_create_fixed_size_strategy(chunker, file);
            break;
        case SLIDING_WINDOW_CHUNKING:
            break;
        case SEMANTIC_CHUNKING:
            break;
        default:
            // TODO: fixed size chunking with some default to 512 and that's it
            break;
    }
    return (chunks_t) { .chunks = NULL, .n_chunks = 0};
}