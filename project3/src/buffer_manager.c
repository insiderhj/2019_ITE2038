#include "bpt.h"

buffer_t* buf_pool;

int init_db(int num_buf) {
    // pool already exists
    if (buf_pool) return CONFLICT;

    // size is equal to or less than zero
    if (num_buf <= 0) return BAD_REQUEST;

    buf_pool = (buffer_t*)calloc(num_buf, sizeof(buffer_t));
    return 0;
}
