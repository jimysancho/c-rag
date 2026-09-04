#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "openai.h"


char *json_escape(char *text) {
    size_t extra = 0;

    for (const char *p = text; *p != '\0'; p++) {
        switch (*p) {
            case '"':
            case '\\':
            case '\n':
            case '\r':
            case '\t':
            case '\b':
            case '\f':
                extra++;
                break;
        }
    }

    size_t output_size = strlen(text) + extra + 1;

    char *escaped = malloc(output_size);

    if (escaped == NULL) {
        printf("Could not allocate memory to escape json\n");
        exit(1);
    }

    const char *src = text;
    char *dst = escaped;

    while (*src != '\0') {
        switch (*src) {
            case '\n':
                *dst++ = '\\';
                *dst++ = 'n';
                break;

            case '\r':
                *dst++ = '\\';
                *dst++ = 'r';
                break;

            case '\t':
                *dst++ = '\\';
                *dst++ = 't';
                break;

            case '"':
                *dst++ = '\\';
                *dst++ = '"';
                break;

            case '\\':
                *dst++ = '\\';
                *dst++ = '\\';
                break;

            default:
                *dst++ = *src;
                break;
        }

        src++;
    }

    *dst = '\0';

    return escaped;

}

size_t write_callback(void *contents,
                      size_t size,
                      size_t nmemb,
                      void *userp) {
    size_t total = size * nmemb;
    response_t *response = userp;

    char *ptr = realloc(response->data, response->size + total + 1);
    if (ptr == NULL) {
        return 0;
    }

    response->data = ptr;

    memcpy(
        response->data + response->size,
        contents,
        total
    );

    response->size += total;
    response->data[response->size] = '\0';

    return total;
}


char *compute_embedding(char *content) {
    CURL *curl;
    CURLcode res;

    response_t response = {
        .data = NULL,
        .size = 0
    };

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();

    if (curl == NULL) {
        fprintf(stderr, "Failed to initialize curl\n");
        exit(1);
    }

    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://api.openai.com/v1/embeddings");

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    const char *api_key = getenv("OPENAI_API_KEY");

    char auth_header[1024];
    snprintf(
        auth_header,
        sizeof(auth_header),
        "Authorization: Bearer %s",
        api_key
    );

    headers = curl_slist_append(
        headers,
        auth_header
    );

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                     &response);
    
    char *escaped_content = json_escape(content);
    int len = snprintf(
        NULL,
        0,
        "{"
            "\"model\":\"text-embedding-3-small\","
            "\"input\":\"%s\""
        "}",
        escaped_content
    );

    if (len < 0) {
       printf("Could not compute length of input\n");
       exit(1);
    }

    char *json = malloc((size_t)len + 1);

    if (json == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    snprintf(
        json,
        (size_t)len + 1,
        "{"
            "\"model\":\"text-embedding-3-small\","
            "\"input\":\"%s\""
        "}",
        escaped_content
    );

    printf("Json body being sent: %s\n", json);

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr,
                "Request failed: %s\n",
                curl_easy_strerror(res));
    } else {
        printf("Response (%zu bytes):\n%s\n",
               response.size,
               response.data);
    }

    printf("Response: %s\n", response.data);

    free(json);
    free(escaped_content);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return response.data;
}