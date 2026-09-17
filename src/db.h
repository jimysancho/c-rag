#ifndef DB_H
#define DB_H

#include <stdio.h>
#include "chunk.h"


typedef struct __db_t {
    char *path; // path where the db is
    size_t __created;
} db_t;


typedef struct __bulk_retrieve_t {
    pthread_mutex_t *lock;
    chunks_t *chunks;
    size_t index;
    char *hash;
    db_t *db;
} bulk_retrieve_t;


typedef struct __bulk_insert_arg_t {
    size_t *count;
    pthread_mutex_t *lock;
    db_t *db;
    chunk_t *chunk;
} bulk_insert_arg_t;


void init_db(db_t *db);

// TODO: once we have sqlite we can save generic void data instead of assuming a chunk
size_t db_insert(db_t *db, chunk_t *chunk);
chunk_t *db_retrieve(db_t *db, char *hash);
size_t db_delete(db_t *db, chunk_t *chunk);

size_t db_bulk_insert(db_t *db, chunks_t chunks);
chunks_t db_bulk_retrieve(db_t *db, char **hash, size_t size);
size_t db_bulk_delete(db_t *db, chunks_t *chunks);

#endif