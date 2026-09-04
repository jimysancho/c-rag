#ifndef OPENAI_H
#define OPENAI_H

#include <stdio.h>

typedef struct {
    char *data;
    size_t size;
} response_t;


size_t write_callback(void *contents,
                             size_t size,
                             size_t nmemb,
                             void *userp);

                             
char *compute_embedding(char *content);


#endif