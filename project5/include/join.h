#ifndef __JOIN_H__
#define __JOIN_H__

#include "bpt.h"

/**
 * Open file to save join result
 * 
 * @param pathname pathname of opening file
 * @returns fd if success. Otherwise, return negative value
 */
int open_join_file(char* pathname);

/**
 * Find leftmost leaf node of table
 * 
 * @param table_id unique table id of searching table
 * @param root_num root number of current table
 * @returns leftmost leaf's page number
 */
pagenum_t find_leftmost_leaf(int table_id, pagenum_t root_num);

/**
 * Increase current index.
 * If idx >= number of keys, move current buffer to right sibling and set idx 0
 * 
 * @param buf using buffer
 * @param idx current index
 */
void advance(buffer_t** buf, int* idx);

/**
 * Write join result to file.
 * 
 * @param fd fd to write join result
 * @param key_1 writing key of first table
 * @param value_1 writing value of first table
 * @param key_2 writing key of second table
 * @param value_2 writing value of second table
 */
void write_csv(int fd, int64_t key_1, char* value_1, int64_t key_2, char* value_2);

/**
 * Join tables.
 * 
 * @param table_id_1 table id of first table
 * @param table_id_2 table id of second table
 * @param pathname pathname of file to save result
 * @returns 0 if success. Otherwise, return negative value
 */
int join_table(int table_id_1, int table_id_2, char * pathname);

#endif
