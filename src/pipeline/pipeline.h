#ifndef PIPELINE_H
#define PIPELINE_H

#include "../json.h"
#include "../ds.h"


void get_embedding_from_json(json_object_t *j, float embedding[1536]);

#endif