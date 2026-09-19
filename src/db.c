#include "db.h"

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_PREFIX
#define N_THREADS 10
#define CHILDREN_CAP 5
#define BUCKETS 16


size_t __create_folder_if_not_exists(char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 0;
    }
    if (errno == ENOENT) {
        if ((mkdir(path, 0777)) == 0) {
            printf("Folder %s created\n", path);
            return 0;
        } else {
            perror("Error creating folder");
            return 1;
        }
    }
    printf("Folder %s created\n", path);
    return 0;
}


char *__path_join(char *s1, char *s2) {
    size_t s1_len = strlen(s1);
    size_t size = s1_len + strlen(s2) + 1 + 1;
    char *s1_copy = malloc(size);
    if (!s1_copy) exit(1);
    memcpy(s1_copy, s1, s1_len);
    // strcat needs the destination to already be a valid string
    s1_copy[s1_len] = '\0';
    s1_copy = strcat(s1_copy, "/");
    s1_copy = strcat(s1_copy, s2);
    s1_copy[size - 1] = '\0';
    return s1_copy;
}


void __create_db_folder(db_t *db) {

    if (db->__created != 0) return;

    if (__create_folder_if_not_exists(db->path) != 0) {
        printf("Could not create db\n");
        exit(1);
    }
    printf("Db created\n");
    db->__created = 1;
    return;
}


void init_db(db_t *db) {
    __create_db_folder(db);
    db->keys = malloc(sizeof(hash_map_t));
    if (!db->keys) exit(1);
    hash_map_init(db->keys, BUCKETS);
    pthread_mutex_init(&db->lock, NULL);
}


void destroy_db(db_t *db) {
    hash_map_free(db->keys);
    pthread_mutex_destroy(&db->lock);
}


size_t db_insert(db_t *db, chunk_t *chunk) {
    __create_db_folder(db);

    //TODO: check if the folder exists or not. If it does, then we need to update, not insert
    char *chunk_path = __path_join(db->path, chunk->hash);
    if (__create_folder_if_not_exists(chunk_path) != 0) {
        printf("Could not create folder for chunk: %s\n", chunk->hash);
        free(chunk_path);
        exit(1);
    }

    char *chunk_content_path = __path_join(chunk_path, "content");
    FILE *file = fopen(chunk_content_path, "w");

    if (!file) {
        printf("Could not create chunk file %s\n", chunk_content_path);
        free(chunk_path);
        free(chunk_content_path);
        exit(1);
    }

    fwrite(chunk->content, strlen(chunk->content), 1, file);
    fclose(file);

    // embedding
    char *chunk_embedding_path = __path_join(chunk_path, "embedding");
    FILE *emb_file = fopen(chunk_embedding_path, "w");

    if (!emb_file) {
        printf("Could not create embedding chunk\n");
        free(chunk_path);
        free(chunk_content_path);
        free(chunk_embedding_path);
        exit(1);
    }

    for (size_t n = 0; n < 1536; n++) {
        fprintf(emb_file, "%f", chunk->embedding[n]);

        if (n < 1535) {
            fprintf(emb_file, ",");
        }
    }
    fclose(emb_file);

    char *chunk_rel_path = __path_join(chunk_path, "relationships");
    FILE *rel_file = fopen(chunk_rel_path, "w");

    if (!rel_file) {
        printf("Could not create relationship chunk\n");
        free(chunk_path);
        free(chunk_content_path);
        free(chunk_embedding_path);
        free(chunk_rel_path);
        exit(1);
    }

    if (chunk->prev_ref && chunk->prev_ref->as.chunk) {
        chunk_type_is_correct(chunk->prev_ref, FULL);
        fprintf(rel_file, "%s", chunk->prev_ref->as.chunk->hash);
    } else {
        fprintf(rel_file, "%s", "null");
    }
    fprintf(rel_file, "%s", "\n");
    if (chunk->next_ref && chunk->next_ref->as.chunk) {
        chunk_type_is_correct(chunk->prev_ref, FULL);
        fprintf(rel_file, "%s", chunk->next_ref->as.chunk->hash);
    } else {
        fprintf(rel_file, "%s", "null");
    }
    fprintf(rel_file, "%s", "\n");
    if (chunk->parent_ref && chunk->parent_ref->as.chunk) {
        chunk_type_is_correct(chunk->prev_ref, FULL);
        fprintf(rel_file, "%s", chunk->parent_ref->as.chunk->hash);
        fprintf(rel_file, "%s", "\n");
    }

    if (chunk->children_ref.chunk_refs) {
        for (size_t n_c = 0; n_c < chunk->children_ref.size; n_c++) {
            chunk_type_is_correct(chunk->children_ref.chunk_refs[n_c], FULL);
            fprintf(rel_file, "%s", chunk->children_ref.chunk_refs[n_c]->as.chunk->hash);
            fprintf(rel_file, "%s", "\n");
        }
    }

    fclose(rel_file);

    char *chunk_metadata_path = __path_join(chunk_path, "metadata");
    FILE *chunk_metadata_file = fopen(chunk_metadata_path, "w");
    if (!chunk_metadata_file) {
        printf("Could not create metadata chunk file\n");
        free(chunk_path);
        free(chunk_content_path);
        free(chunk_embedding_path);
        free(chunk_rel_path);
        free(chunk_metadata_path);
        exit(1);
    }

    fprintf(chunk_metadata_file, "%ld", chunk->metadata.bytes);
    fprintf(chunk_metadata_file, "%s", "\n");
    fprintf(chunk_metadata_file, "%s", chunk->metadata.path);
    fprintf(chunk_metadata_file, "%s", "\n");
    fprintf(chunk_metadata_file, "%u", chunk->metadata.strategy);
    fprintf(chunk_metadata_file, "%s", "\n");
    fprintf(chunk_metadata_file, "%zu", chunk->metadata.start);
    fprintf(chunk_metadata_file, "%s", "\n");
    fprintf(chunk_metadata_file, "%zu", chunk->metadata.end);

    fclose(chunk_metadata_file);

    char *emb_path = strdup(chunk_embedding_path);

    free(chunk_path);
    free(chunk_content_path);
    free(chunk_embedding_path);
    free(chunk_rel_path);
    free(chunk_metadata_path);

    pthread_mutex_lock(&db->lock);
    //NOTE: we need to pass a copy so later on we can free things properly
    hash_map_insert(db->keys, strdup(chunk->hash), (void *)emb_path, sizeof(chunk->hash));
    pthread_mutex_unlock(&db->lock);

    return 1;
}


void load_embeddings(char *path, chunk_t *chunk) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        printf("Path %s does not exist\n", path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("fseek");
        exit(1);
    }
    long n_file_bytes = ftell(file);
    fseek(file, 0, 0);
    char *contents = malloc(n_file_bytes);

    size_t nread = fread(contents, 1, n_file_bytes, file);
    fclose(file);

    contents[nread] = '\0';
    size_t dim = 0;

    char *savepointer;
    for (char *word = strtok_r(contents, ",", &savepointer); word != NULL; word = strtok_r(NULL, ",", &savepointer)) {
        chunk->embedding[dim] = atof(word);
        dim++;
    }
    free(contents);
}


chunk_t *db_retrieve(db_t *db, char *hash) {
    __create_db_folder(db);
    // check existence of chunk 
    char *chunk_path = __path_join(db->path, hash);
    DIR *dir = opendir(chunk_path);
    if (errno == ENOENT) {
        free(chunk_path);
        return NULL;
    }

    closedir(dir);

    char *content_path, *embedding_path, *rel_path, *metadata_path;
    content_path = __path_join(chunk_path, "content");
    embedding_path = __path_join(chunk_path, "embedding");
    rel_path = __path_join(chunk_path, "relationships");
    metadata_path = __path_join(chunk_path, "metadata");

    FILE *content_file, *rel_file, *metadata_file;
    content_file = fopen(content_path, "r");
    rel_file = fopen(rel_path, "r");
    metadata_file = fopen(metadata_path, "r");

    if (!content_file || !rel_file || !metadata_file) {
        printf("Corruption of chunk %s. Missing content_file\n", chunk_path);
        free(chunk_path);
        free(content_path);
        free(embedding_path);
        free(metadata_path);

        if (content_file) fclose(content_file);
        if (rel_file) fclose(rel_file);
        if (metadata_file) fclose(metadata_file);
        exit(1);
    }

    chunk_t *chunk = malloc(sizeof(chunk_t));

    if (!chunk) {
        free(chunk_path);
        free(content_path);
        free(embedding_path);
        free(metadata_path);

        if (content_file) fclose(content_file);
        if (rel_file) fclose(rel_file);
        if (metadata_file) fclose(metadata_file);
        exit(1);
    }

    chunk_init(chunk, HASH);
    char *hash_copy = strdup(hash);
    for (size_t i = 0; i < 65; i++) {
        chunk->hash[i] = hash_copy[i];
    }
    chunk->hash[64] = '\0';
    free(hash_copy);

    chunk_metadata_t metadata;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // we have 5 lines
    size_t n = 0;
    while ((read = getline(&line, &len, metadata_file)) != -1) {
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        switch (n) {
            case 0:
                // 0 -> bytes
                metadata.bytes = (long)atoi(line);
                break;
            case 1:
                // 1 -> path
                metadata.path = strdup(line);
                break;
            case 2:
                // 2-> strategy
                switch (atoi(line)) {
                    case FIXED_SIZE_CHUNKING:
                        metadata.strategy = FIXED_SIZE_CHUNKING;
                        break;
                    case SLIDING_WINDOW_CHUNKING:
                        metadata.strategy = SLIDING_WINDOW_CHUNKING;    
                        break;
                    case SEMANTIC_CHUNKING:
                        metadata.strategy = SEMANTIC_CHUNKING;
                        break;
                    case STRUCTURAL_CHUNKING:
                        metadata.strategy = STRUCTURAL_CHUNKING;
                        break;
                    default:
                        break;
                }
                break;
            case 3:
                // start
                metadata.start = (size_t)atoi(line);
                break;
            case 4:
                // end
                metadata.end = (size_t)atoi(line);
                break;
            default:
                break;
        }
        n++;
    }
    free(line);
    chunk->metadata = metadata;

    char *contents = NULL;
    if (chunk->metadata.bytes > 0) {
        contents = malloc(chunk->metadata.bytes + 1);
        if (!contents) {
            fclose(content_file);
            fclose(rel_file);
            fclose(metadata_file);

            free(chunk_path);
            free(content_path);
            free(embedding_path);
            free(metadata_path);
            free(rel_path);

            free(chunk->metadata.path);
            exit(1);
        }
        size_t nread = fread(contents, 1, chunk->metadata.bytes, content_file);
        if (nread != (size_t)chunk->metadata.bytes) {
            printf("bytes read vs actually read: %zu %zu\n", (size_t)chunk->metadata.bytes + 1, nread);
            fclose(content_file);
            fclose(rel_file);
            fclose(metadata_file);

            free(chunk_path);
            free(content_path);
            free(embedding_path);
            free(metadata_path);
            free(rel_path);

            free(chunk->metadata.path);
            exit(1);
        }
        contents[chunk->metadata.bytes] = '\0';
    }
    chunk->content = contents;

    load_embeddings(embedding_path, chunk);

    char *rel_line = NULL;
    ssize_t rel_read;

    chunks_ref_t children = {0};
    n = 0;

    while ((rel_read = getline(&rel_line, &len, rel_file)) != -1) {
        if (rel_read > 0 && rel_line[rel_read - 1] == '\n') {
            rel_line[rel_read - 1] = '\0';
        }
        switch (n) {
            case 0:
                // 0 -> prev
                memcpy(chunk->prev_ref->as.hash, rel_line, 64);
                chunk->prev_ref->as.hash[64] = '\0';
                break;
            case 1:
                // 1 -> next
                memcpy(chunk->next_ref->as.hash, rel_line, 64);
                chunk->next_ref->as.hash[64] = '\0';
                break;
            case 2:
                // 2-> parent
                memcpy(chunk->parent_ref->as.hash, rel_line, 64);
                chunk->parent_ref->as.hash[64] = '\0';
                break;
            // the rest are children
            case 3: {
                    if (strcmp(rel_line, "\n") == 0) break;
                    children.chunk_refs = malloc(sizeof(chunk_t *) * CHILDREN_CAP);
                    memcpy(children.chunk_refs[children.size++]->as.hash, rel_line, 64);
                    children.chunk_refs[children.size]->as.hash[64] = '\0';
                }
                // first children
            default:
                // rest of children
                {
                    if (strcmp(rel_line, "\n") == 0) break;
                    if (children.size >= CHILDREN_CAP) {
                        children.chunk_refs = realloc(children.chunk_refs, children.size * 2);
                    }
                    memcpy(children.chunk_refs[children.size++]->as.hash, rel_line, 64);
                    children.chunk_refs[children.size]->as.hash[64] = '\0';
                }
                break;
        }
        n++;
    }
    free(rel_line);

    free(chunk_path);
    free(content_path);
    free(embedding_path);
    free(metadata_path);
    free(rel_path);

    fclose(content_file);
    fclose(rel_file);
    fclose(metadata_file);

    return chunk;
}


size_t db_delete(db_t *db, chunk_t *chunk) {
    __create_db_folder(db);
    (void)chunk;
    return 0;
}


void _add_chunk(chunks_t *chunks, 
                chunk_t *chunk, 
                size_t index,
                pthread_mutex_t *lock) {
    pthread_mutex_lock(lock);
    chunks->chunks[index] = chunk;
    chunks->n_chunks++;
    pthread_mutex_unlock(lock);
}


void *retrieve_and_add(void *b_arg) {
    bulk_retrieve_t *bulk_retrieve_arg = (bulk_retrieve_t *)b_arg;
    chunk_t *chunk = db_retrieve(bulk_retrieve_arg->db, bulk_retrieve_arg->hash);
    if (!chunk) {
        printf("Could not retrieve %s\n", bulk_retrieve_arg->hash);
        return NULL;
    }
    _add_chunk(
        bulk_retrieve_arg->chunks, 
        chunk, 
        bulk_retrieve_arg->index, 
        bulk_retrieve_arg->lock
    );
    return NULL;
}


void *insert_and_add(void *b_arg) {
    bulk_insert_arg_t *bulk_insert_arg = (bulk_insert_arg_t *)b_arg;
    db_insert(bulk_insert_arg->db, bulk_insert_arg->chunk);
    pthread_mutex_lock(bulk_insert_arg->lock);
    (*bulk_insert_arg->count)++;
    pthread_mutex_unlock(bulk_insert_arg->lock);
    return NULL;
}


size_t db_bulk_insert(db_t *db, chunks_t chunks) {
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    size_t n_threads = N_THREADS > chunks.n_chunks ? chunks.n_chunks : N_THREADS;
    pthread_t threads[n_threads];
    size_t count = 0;
    size_t index = 0;

    bulk_insert_arg_t **args = malloc(sizeof(bulk_insert_arg_t *) * chunks.n_chunks);

    while (count != chunks.n_chunks) {
        for (size_t p_n = 0; p_n < n_threads; p_n++) {
            bulk_insert_arg_t *arg = malloc(sizeof(bulk_insert_arg_t));
            if (!arg) exit(1);
            arg->chunk = chunks.chunks[index];
            arg->count = &count;
            arg->db = db;
            arg->lock = &lock;
            args[index] = arg;
            pthread_create(&threads[p_n], NULL, insert_and_add, (void *)arg);
            index++;
        }

        for (size_t p_n = 0; p_n < n_threads; p_n++) {
            pthread_join(threads[p_n], NULL);
        }
    }

    for (size_t i = 0; i < chunks.n_chunks; i++) {
        free(args[i]);
    }
    free(args);
    printf("%zu inserted chunks\n", count);
    return count;
}


chunks_t db_bulk_retrieve(db_t *db, char **hash, size_t size) {
    chunks_t retrieve_chunks = {
        .chunks = malloc(sizeof(chunk_t *) * size),
        .n_chunks = 0,
    };
    if (!retrieve_chunks.chunks) exit(1);

    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    size_t n_threads = N_THREADS > size ? size : N_THREADS;
    pthread_t threads[n_threads];
    size_t index = 0;
    bulk_retrieve_t **args = malloc(sizeof(bulk_retrieve_t *) * size);

    while (retrieve_chunks.n_chunks != size) {
        for (size_t p_n = 0; p_n < n_threads; p_n++) {
            bulk_retrieve_t *arg = malloc(sizeof(bulk_retrieve_t));
            if (!arg) exit(1);
            arg->chunks = &retrieve_chunks;
            arg->db = db;
            arg->hash = hash[index];
            arg->lock = &lock;
            arg->index = index;
            pthread_create(&threads[p_n], NULL, retrieve_and_add, (void *)arg);
            if (index >= size) {
                printf("somethign went wrong -> %zu\n", index);
                exit(1);
            }
            args[index] = arg;
            index++;
        }

        for (size_t p_n = 0; p_n < n_threads; p_n++) {
            pthread_join(threads[p_n], NULL);
        }
    }

    for (size_t i = 0; i < size; i++) {
        free(args[i]);
    }
    free(args);

    pthread_mutex_destroy(&lock);
    return retrieve_chunks;
    
}
size_t db_bulk_delete(db_t *db, chunks_t *chunks);