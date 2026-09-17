#ifndef CHUNK_H
#define CHUNK_H

#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include "parser.h"

#define EMBEDDING_DIMENSION 1536
#define SHA256_HEX_LENGTH 64


typedef enum __CHUNK_STRATEGY {
    FIXED_SIZE_CHUNKING,
    SLIDING_WINDOW_CHUNKING,
    SEMANTIC_CHUNKING,
    STRUCTURAL_CHUNKING
} CHUNK_STRATEGY;


typedef struct __fixed_size_params_t {
    size_t size;
} fixed_size_params_t;


typedef struct __sliding_window_params_t {
    size_t overlap;
    size_t window_size;
} sliding_window_params_t;


typedef struct __semantic_chunking_params_t {
    float similarity;
    size_t max_chunk_size;
    size_t min_chunk_size;
} semantic_chunking_params_t;


typedef struct __chunking_strategy_params_t {
    CHUNK_STRATEGY strategy;
    union {
        fixed_size_params_t fixed_size_params;
        sliding_window_params_t sliding_window_params;
        semantic_chunking_params_t semantic_chunking_params;
    } as;
} chunking_strategy_params_t;


typedef struct __chunker_t {
    CHUNK_STRATEGY strategy;
    chunking_strategy_params_t params;
} chunker_t;


typedef struct __chunk_metadata_t {
    long bytes;
    char *path;
    CHUNK_STRATEGY strategy;
    size_t start;
    size_t end;
    // TODO: maybe when it was inserted, if it was modified, etc
} chunk_metadata_t;


typedef struct __chunk_t chunk_t;

typedef struct __chunks_t {
    chunk_t **chunks;
    size_t n_chunks;
} chunks_t;


typedef enum {
    FULL,
    HASH
} ref_type;


typedef struct __chunk_ref_t {
    ref_type type;
    union {
        struct __chunk_t *chunk;
        char hash[SHA256_HEX_LENGTH + 1];
    } as;
} chunk_ref_t;


typedef struct __chunks_ref_t {
    chunk_ref_t **chunk_refs;
    size_t size;
} chunks_ref_t;


typedef struct __chunk_t {
    uuid_t chunk_id;
    char hash[SHA256_HEX_LENGTH + 1];
    char *content;
    float embedding[EMBEDDING_DIMENSION];
    // FIXME: this model is not right to use for the db, due to the fact that this implies that we need to fetch all of them
    // in order to rebuild them
    // a good model would be a union of pointer and hash. that way we can do next->as.hash or something like that
    // and when retrieving it, doing it as hash
    chunk_ref_t *next_ref;
    chunk_ref_t *prev_ref;
    chunk_ref_t *parent_ref;
    chunks_ref_t children_ref;
    chunk_metadata_t metadata;
} chunk_t;


void chunk_init(chunk_t *chunk, ref_type type);
chunks_t chunks_create(chunker_t *chunker, file_t *file);
void chunk_free(chunk_t *chunk);
void chunks_free(chunks_t chunks);
void chunk_visualize(chunk_t *chunk, ref_type type);
void chunks_visualize(chunks_t chunks, ref_type type);
void chunk_type_is_correct(chunk_ref_t *chunk_ref, ref_type expected);

#endif