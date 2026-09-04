#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../src/ds.h"


static void test_ll_push() {
    ll_t ll;
    ll_init(&ll);
    
    int x1 = 5;
    int x2 = 10;

    ll_push(&ll, (void *)&x1);
    ll_push(&ll, (void *)&x2);

    assert((int *)ll.head->data == &x1);
    assert((int *)ll.tail->data == &x2);
    assert(ll.head->next->data == &x2);
    assert(ll.size == 2);
}


static void test_ll_hash_map_insert() {
    hash_map_t h;
    size_t n_buckets = 10;
    hash_map_init(&h, n_buckets);
    assert(h.n_buckets == n_buckets);
    for (size_t n = 0; n < n_buckets; n++) {
        ll_t *ll = h.buckets[n];
        assert(ll == NULL);
    }

    char *test_string = "hello there";
    size_t bucket = hash_string(test_string, n_buckets);
    assert(bucket == 2);
    assert(bucket <= n_buckets && bucket >= 0);

    char *value = "it's a pleasure to meet you!";

    h_object_t *inserted_obj = hash_map_insert(&h, test_string, (void *)value, sizeof(value));
    ll_t *ll2 = h.buckets[2];
    assert(ll2 != NULL);
    assert(h.n_keys == 1);
    assert(inserted_obj != NULL);

    h_object_t *obj = hash_map_get(&h, test_string);
    assert(obj != NULL);
    assert(obj->key == test_string);
    assert((char *)obj->value == value);
    assert(sizeof((char *)obj->value) == obj->size);

    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        hash_map_insert(
            &h,
            "hello there",
            (void *)"some other value",
            sizeof("some other value")
        );
        exit(0);
    }

    int status;
    waitpid(pid, &status, 0);

    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 1);

}

int main() {
    test_ll_push();
    test_ll_hash_map_insert();
    return 0;
}