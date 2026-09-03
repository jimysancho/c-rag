#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"


#define FILE_PREVIEW_SIZE 100


file_t *_load_txt_contents(char *path) {
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

    fread(contents, n_file_bytes, 1, file);
    fclose(file);

    file_t *f = malloc(sizeof(file_t));
    f->path = path;
    f->bytes = n_file_bytes;
    f->contents = contents;
    
    if (strstr(f->path, ".txt")) {
        f->extension = TXT;
    } else if (strstr(f->path, ".pdf")) {
        f->extension = PDF;
    } else {
        printf("Unknown extension of file %s\n", f->path);
    }

    return f;
}


file_t *_load_pdf_contents(char *path) {
    (void)path;
    return NULL;
}


file_t *load_file_contents(char *path) {

    file_t *file = NULL;
    if (strstr(path, ".txt")) {
        file = _load_txt_contents(path);
    } else if (strstr(path, ".pdf")) {
        file = _load_pdf_contents(path); 
    } else {
        printf("Unknown extension of file %s\n", path);
        exit(1);
    }

    return file;
}


void free_files(file_t **files, size_t n_files) {
    for (size_t i = 0; i < n_files; i++) {
        file_t *file = files[i];
        free(file->contents);
        free(file);
    }
}


void file_visualize(file_t *file) {
    printf("File: %s\n", file->path);
    printf("Size: %ld\n", file->bytes);
    printf("Preview (100):\n");
    for (size_t i = 0; i < FILE_PREVIEW_SIZE; i++) {
        printf("%c", file->contents[i]);
    }
    printf("...\n");
}