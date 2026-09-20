#include "generation.h"
#include "pipeline.h"
#include "openai.h"


//TODO: improve this
char *SYSTEM_PROMPT = "Generate the most accurate answer based on the provided information. Only the provided information, regardless of the user's question";


generation_result_t generation_pipeline_run(generation_pipeline_t pipeline) {
    char *response = compute_answer(SYSTEM_PROMPT, pipeline.query);
    char *content = get_content_from_response_dict(response);
    return (generation_result_t) {
        .response = content
    };
}