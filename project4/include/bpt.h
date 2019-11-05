#ifndef __BPT_H__
#define __BPT_H__

#include "types.h"
#include "disk_space_manager.h"
#include "buffer_manager.h"

// open
int open_table(char* pathname);

// util
int cut(int len);

// find
pagenum_t find_leaf(int table_id, pagenum_t root, int64_t key);
int db_find(int table_id, int64_t key, char* ret_val);

// insert
pagenum_t start_new_tree(buffer_t* header, int64_t key, char* value);
pagenum_t insert_into_new_root(buffer_t* header, buffer_t* left,
                               int64_t key, buffer_t* right);
int get_left_index(buffer_t* parent, pagenum_t left_num);
void insert_into_node(int table_id, buffer_t* parent, int left_index, int64_t key, pagenum_t right_num);
pagenum_t insert_into_node_after_splitting(buffer_t* header, pagenum_t root_num,
                                           buffer_t* parent,
                                           int left_index, int64_t key, pagenum_t right_num);
pagenum_t insert_into_parent(buffer_t* header, pagenum_t root_num, buffer_t* left,
                             int64_t key, buffer_t* right);
void insert_into_leaf(int table_id, buffer_t* leaf, int64_t key, char* value);
pagenum_t insert_into_leaf_after_splitting(buffer_t* header_page, pagenum_t root_num, buffer_t* leaf,
                                           int64_t key, char* value);
int db_insert(int table_id, int64_t key, char* value);

// delete
void remove_entry_from_leaf(buffer_t* node, int64_t key);
void remove_entry_from_node(buffer_t* node, int64_t key);
pagenum_t adjust_root(buffer_t* header, pagenum_t root_num);
int get_neighbor_index(buffer_t* parent, pagenum_t node_num);
pagenum_t coalesce_nodes(buffer_t* header_page, pagenum_t root_num, buffer_t* node,
                         buffer_t* neighbor, int neighbor_index,
                         int64_t k_prime);
void redistribute_nodes(int table_id, buffer_t* node, buffer_t* neighbor,
                        int neighbor_index, int k_prime_index, int64_t k_prime);
pagenum_t delete_entry(buffer_t* header_page, pagenum_t root_num, pagenum_t node_num, int64_t key);
int db_delete(int table_id, int64_t key);

// print
void enqueue(Queue* q, pagenum_t data);
pagenum_t dequeue(Queue* q);
void print_tree(int table_id);

#endif
