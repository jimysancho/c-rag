#ifndef PARSER_H
#define PARSER_H


typedef enum {
    PDF,
    TXT
} EXTENSION;


typedef struct __file_t {
    char *path;
    char *contents;
    long bytes;
    EXTENSION extension;
} file_t;


file_t *load_file_contents(char *path);
void free_files(file_t **files, size_t n_files);
void file_visualize(file_t *file);

#endif