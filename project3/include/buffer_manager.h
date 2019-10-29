#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include "bpt.h"

int init_db(int num_buf);
void set_mru(int buf_num);

buffer_t* find_buf(int table_id, pagenum_t pagenum);

buffer_t* get_buf(int table_id, pagenum_t pagenum);
buffer_t* add_buf();

buffer_t* buf_alloc_page(int table_id);
buffer_t* buf_free_page(int table_id, pagenum_t pagenum);

void flush_buf(buffer_t* buf);

void set_root(int table_id, int root_num);

int close_table(int table_id);
int shutdown_db();

#endif
