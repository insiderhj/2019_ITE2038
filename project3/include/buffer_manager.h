#ifndef __BUFFER_MANAGER_H__
#define __BUFFER_MANAGER_H__

#include "bpt.h"

int init_db(int num_buf);
int buf_read_table(char* pathname);

void set_mru(int buf_num);

int find_buf(int table_id, pagenum_t pagenum);

buffer_t* get_buf(int table_id, pagenum_t pagenum, uint32_t is_dirty);
int add_buf();

buffer_t* buf_alloc_page(int table_id);
void buf_free_page(int table_id, pagenum_t pagenum);

void flush_buf(int buf_num);

void set_root(int table_id, int root_num);

int close_table(int table_id);
int shutdown_db();

void print_buf();

#endif
