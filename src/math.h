#ifndef MATH_H
#define MATH_H

#include <stdio.h>


float compute_similarity(float v1[1536], float v2[1536]) {
    float sim = 0;
    for (size_t i = 0; i < 1536; i++) {
        sim += v1[i] * v2[i];
    }
    return sim;
}

#endif