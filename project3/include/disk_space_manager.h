#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "bpt.h"

// file manager
int check_pathname(char* pathname);
int file_open(char* pathname);
int file_read_init(int table_id, buffer_t* header_page);

void file_read_page(int table_id, pagenum_t pagenum, buffer_t* dest);
void file_write_page(int table_id, pagenum_t pagenum, const buffer_t* src);

void file_set_parent(int table_id, pagenum_t pagenum, pagenum_t parent_num);

#endif
