#ifndef GENERATION_H
#define GENERATION_H


typedef struct __generation_pipeline_t {
    // maybe openai params. For now it can be emtpy actuallly
    char *model; // enum better
    char *query;
} generation_pipeline_t;


typedef struct __generation_result_t {
    char *response;
} generation_result_t;


generation_result_t generation_pipeline_run(generation_pipeline_t pipeline);

#endif