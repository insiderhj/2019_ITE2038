#ifndef __JOIN_H__
#define __JOIN_H__

#include "bpt.h"

int open_join_file(char* pathname);
pagenum_t find_leftmost_leaf(int table_id, pagenum_t root_num);
void advance(buffer_t** buf, int* idx);
void write_csv(int fd, int64_t key_1, char* value_1, int64_t key_2, char* value_2);
int join_table(int table_id_1, int table_id_2, char * pathname);

#endif
