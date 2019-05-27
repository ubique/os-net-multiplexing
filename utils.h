#include <stdio.h>    // fprintf, stderr
#include <stdlib.h>   // free

#include "common.h"

#define MAX(x, y) (x) > (y) ? (x) : (y)

#define err_log(fmt, ...) fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)

#define SCOPED_PEER __attribute__((cleanup(cleanup_peer)))
#define SCOPED_MEM __attribute__((cleanup(cleanup_mem)))

static inline void
cleanup_peer(struct cmn_peer **peer) {
    cmn_peer_destroy(*peer);
}

// Cleanup_mem might be somewhat unsafe,
// because sizeof(T*) != sizeof(void*) in general.
// Let's make some assertions
_Static_assert(sizeof(void *) == sizeof(char *), "can't have generic memory cleaner");
_Static_assert(sizeof(void *) == sizeof(short *), "can't have generic memory cleaner");
_Static_assert(sizeof(void *) == sizeof(int *), "can't have generic memory cleaner");

static inline void
cleanup_mem(void *pmem) {
    void *mem = *(void **)pmem;
    free(mem);
}

int
get_open_max();

