#ifndef PIPELINE_H
#define PIPELINE_H

#include "../json.h"
#include "../ds.h"
#include "retrieval.h"
#include "generation.h"


typedef struct __pipeline_t {
    retrieval_pipeline_t *retrieval_pipeline;
    generation_pipeline_t *generation_pipeline;
} pipeline_t;


typedef struct __pipeline_result_t {
    retrieval_result_t ret_result;
    generation_result_t gen_result;
} pipeline_result_t;


void get_embedding_from_json(json_object_t *j, float embedding[1536]);
char *get_content_from_response_dict(char *response);


pipeline_result_t pipeline_run(pipeline_t pipeline);
#endif