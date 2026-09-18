#include <assert.h>
#include <openssl/sha.h>
#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>


#include "chunk.h"


#define CAPACITY 32


void chunk_type_is_correct(chunk_ref_t *chunk_ref, ref_type expected) {
    assert(chunk_ref->type == expected);
}


void chunk_init(chunk_t *chunk, ref_type type) {
    uuid_generate_random(chunk->chunk_id);
    chunk->hash[0] = '\0';
    chunk->content = NULL;
    memset(chunk->embedding, 0, sizeof(chunk->embedding));
    chunk->children_ref = (chunks_ref_t) {
        .chunk_refs = NULL,
        .size = 0
    };
    chunk->next_ref = malloc(sizeof(chunk_ref_t));
    if (!chunk->next_ref) exit(1);
    chunk->next_ref->type = type;
    chunk->prev_ref = malloc(sizeof(chunk_ref_t));
    if (!chunk->prev_ref) exit(1);
    chunk->prev_ref->type = type;
    chunk->parent_ref = malloc(sizeof(chunk_ref_t));
    if (!chunk->prev_ref) exit(1);
    chunk->parent_ref->type = type;
    chunk->metadata = (chunk_metadata_t) {
        .bytes = 0,
        .path = NULL,
        .strategy = 0
    };
}


void chunk_free(chunk_t *chunk) {
    free(chunk->content);
    free(chunk->prev_ref);
    free(chunk->next_ref);
    free(chunk->parent_ref);
    free(chunk->metadata.path);
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


void chunk_visualize(chunk_t *chunk, ref_type type) {
    char uuid_str[37];

    uuid_unparse_lower(chunk->chunk_id, uuid_str);

    printf("Chunk '%s'\n", uuid_str);
    printf("\t- hash: %s\n", chunk->hash);
    printf("\t- domain: %zu - %zu\n", chunk->metadata.start, chunk->metadata.end);
    printf("\t- content:\n\n%s\n\n", chunk->content);
    printf("\t- embedding: [");
    for (size_t i = 0; i < 10; i++) {
        printf("%f, ", chunk->embedding[i]);
    }
    printf("...]\n");

    chunk_type_is_correct(chunk->prev_ref, type);
    if (type == FULL) {
        if (chunk->prev_ref && chunk->prev_ref->as.chunk) {
            char prev_uuid[37];
            uuid_unparse_lower(chunk->prev_ref->as.chunk->chunk_id, prev_uuid);
            printf("\t- prev: %s\n", prev_uuid);
        } else {
            printf("\t- prev: (null)\n");
        }
    } else {
        if (chunk->prev_ref) {
            printf("\t- prev: %s\n", chunk->prev_ref->as.hash);
        } else {
            printf("\t- prev: (null)\n");   
        }
    }

    chunk_type_is_correct(chunk->next_ref, type);
    if (type == FULL) {        
        if (chunk->next_ref && chunk->next_ref->as.chunk) {
            char next_uuid[37];
            uuid_unparse_lower(chunk->next_ref->as.chunk->chunk_id, next_uuid);
            printf("\t- next: %s\n", next_uuid);
        } else {
            printf("\t- next: (null)\n");
        }
    } else {
        if (chunk->next_ref) {
            printf("\t- next: %s\n", chunk->next_ref->as.hash);
        } else {
            printf("\t- next: (null)\n");   
        }
    }

    chunk_type_is_correct(chunk->parent_ref, type);
    if (type == FULL) {
        if (chunk->parent_ref && chunk->parent_ref->as.chunk) {
            char parent_uuid[37];
            uuid_unparse_lower(chunk->parent_ref->as.chunk->chunk_id, parent_uuid);
            printf("\t- parent: %s\n", parent_uuid);
        }
    } else {
        if (chunk->parent_ref) {
            printf("\t- parent: %s\n", chunk->parent_ref->as.hash);
        } else {
            printf("\t- parent: (null)\n");   
        }
    }

    if (chunk->children_ref.chunk_refs) {
        printf("\t- children:\n");

        for (size_t i = 0; i < chunk->children_ref.size; i++) {
            char child_uuid[37];
            chunk_type_is_correct(chunk->children_ref.chunk_refs[i], type);
            if (type == FULL) {
                uuid_unparse(chunk->children_ref.chunk_refs[i]->as.chunk->chunk_id, child_uuid);
                printf("\t\t- child%zu: %s\n", i, child_uuid);
            } else {
                printf("\t\t- child%zu: %s\n", i, chunk->children_ref.chunk_refs[i]->as.hash);
            }
        }
    }
}


void chunks_visualize(chunks_t chunks, ref_type type) {
    printf("=====================================\n");
    printf("%zu hunks created\n", chunks.n_chunks);
    for (size_t n = 0; n < chunks.n_chunks; n++) {
        chunk_visualize(chunks.chunks[n], type);
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
        perror("Wrong strategy. Expected: FIXED_SIZE_CHUNKING\n");
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
                chunk_init(chunks[n_chunks], FULL);
                chunk->content = chunk_contents;
                chunk_compute_hash(chunk);

                if (n_chunks > 0) {
                    chunk->prev_ref->as.chunk = chunks[n_chunks - 1];
                    chunks[n_chunks - 1]->next_ref->as.chunk = chunk;
                    chunks[n_chunks - 1]->next_ref->as.chunk = chunk;
                    
                }

                chunk->metadata = (chunk_metadata_t) {
                    .bytes = strlen(chunk_contents),
                    .path = strdup(file->path),
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


chunks_t _chunks_create_sliding_window_strategy(chunker_t *chunker, file_t *file) {

    if (chunker->strategy != SLIDING_WINDOW_CHUNKING) {
        perror("Wrong strategy. Expected: SLIDING_WINDOW_CHUNKING\n");
        exit(1);
    }
    chunk_t **chunks = malloc(sizeof(chunk_t *) * CAPACITY);

    if (!chunks) {
        printf("Initial allocation for chunks failed\n"); 
        exit(1);
    }

    size_t n_chunks = 0;
    size_t n_words = 0;
    size_t overlap_start = 0;

    size_t start = 0;
    size_t current_capacity = CAPACITY;

    for (size_t i = 0; i < (size_t)file->bytes; i++) {
        char c = file->contents[i];
        if (c == ' ' || c == '\n' || c == '\t') {
            n_words++;
            if (
                (chunker->params.as.sliding_window_params.window_size - n_words) 
                == chunker->params.as.sliding_window_params.overlap) {
                overlap_start = i + 1;
            }

            if (i > 0 && n_words % chunker->params.as.sliding_window_params.window_size == 0) {
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
                chunk_init(chunks[n_chunks], FULL);
                chunk->content = chunk_contents;
                chunk_compute_hash(chunk);

                if (n_chunks > 0) {
                    chunk->prev_ref->as.chunk = chunks[n_chunks - 1];
                    chunks[n_chunks - 1]->next_ref->as.chunk = chunk;
                }

                chunk->metadata = (chunk_metadata_t) {
                    .bytes = strlen(chunk_contents),
                    .path = strdup(file->path),
                    .strategy = chunker->strategy,
                    .start = start,
                    .end = i
                };

                // we reset n_words for the next chunk, but not with 0, 
                // since the start now will be the one that give us "overlap"
                // number of words already
                n_words = chunker->params.as.sliding_window_params.overlap;

                // in sliding window, the start it's not i + 1, but 
                // "position of -overlap words start of prev chunk"
                start = overlap_start;
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
            printf("Sliding window size chunking strategy for file: %s\n", file->path);
            return _chunks_create_sliding_window_strategy(chunker, file);
            break;
        case SEMANTIC_CHUNKING:
            break;
        default:
            // TODO: fixed size chunking with some default to 512 and that's it
            break;
    }
    return (chunks_t) { .chunks = NULL, .n_chunks = 0};
}