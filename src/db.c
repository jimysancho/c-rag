#include "db.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CHUNK_PREFIX



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
    size_t size = strlen(s1) + strlen(s2) + 1 + 1;
    char *s1_copy = malloc(size);
    if (!s1_copy) exit(1);
    memcpy(s1_copy, s1, strlen(s1));
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


size_t db_insert(db_t *db, chunk_t *chunk) {
    __create_db_folder(db);

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

    if (chunk->prev) {
        fprintf(rel_file, "%s", chunk->prev->hash);
    } else {
        fprintf(rel_file, "%s", "null");
    }
    fprintf(rel_file, "%s", "\n");
    if (chunk->next) {
        fprintf(rel_file, "%s", chunk->next->hash);
    } else {
        fprintf(rel_file, "%s", "null");
    }
    fprintf(rel_file, "%s", "\n");
    if (chunk->parent) {
        fprintf(rel_file, "%s", chunk->parent->hash);
        fprintf(rel_file, "%s", "\n");
    }

    if (chunk->children.chunks) {
        for (size_t n_c = 0; n_c < chunk->children.n_chunks; n_c++) {
            fprintf(rel_file, "%s", chunk->children.chunks[n_c]->hash);
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

    free(chunk_path);
    free(chunk_content_path);
    free(chunk_embedding_path);
    free(chunk_rel_path);
    free(chunk_metadata_path);

    return 1;
}


chunk_t *db_retrieve(db_t *db, chunk_t *chunk) {
    __create_db_folder(db);
    (void)chunk;

    // based on path db->path + chunk->hash, get each file

    // 1. Check whether each file exists or not
    // 2. Load each one
    
    return 0;
}


size_t db_delete(db_t *db, chunk_t *chunk) {
    __create_db_folder(db);
    (void)chunk;
    return 0;
}