#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include "bpt.h"

struct buffer_t {
    page_t frame;
    int table_id;
    pagenum_t page_num;
    uint32_t is_dirty;
    uint32_t is_pinned;
    buffer_t* next;
    buffer_t* prev;
};

int init_db(int num_buf);

#endif
