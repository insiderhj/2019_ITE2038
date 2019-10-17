#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "bpt.h"

// file manager
int file_open(char* pathname);
int file_read_init(int fd, page_t* header_page);

pagenum_t file_alloc_page(int table_id);
void file_free_page(int table_id, pagenum_t pagenum);
void file_read_page(int table_id, pagenum_t pagenum, page_t* dest);
void file_write_page(int table_id, pagenum_t pagenum, const page_t* src);

void file_set_root(int table_id, pagenum_t root_num);
void file_set_parent(int table_id, pagenum_t pagenum, pagenum_t parent_num);

#endif
