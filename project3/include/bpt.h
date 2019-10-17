#ifndef __BPT_H__
#define __BPT_H__

#include "types.h"
#include "disk_manager.h"
#include "buffer_manager.h"

// open
int open_table(char* pathname);

// util
int cut(int len);

// find
pagenum_t find_leaf(int table_id, pagenum_t root, int64_t key);
int db_find(int table_id, int64_t key, char* ret_val);

// insert
pagenum_t make_node(int table_id, page_t* node);
pagenum_t start_new_tree(int table_id, int64_t key, char* value);
pagenum_t insert_into_new_root(int table_id, pagenum_t left_num, page_t* left,
                               int64_t key, pagenum_t right_num, page_t* right);
int get_left_index(page_t* parent, pagenum_t left_num);
void insert_into_node(int table_id, int parent_num, page_t* parent, int left_index, int64_t key, pagenum_t right_num);
pagenum_t insert_into_node_after_splitting(int table_id, pagenum_t root_num,
                                           pagenum_t parent_num, page_t* parent,
                                           int left_index, int64_t key, pagenum_t right_num);
pagenum_t insert_into_parent(int table_id, pagenum_t root_num, pagenum_t left_num, page_t* left,
                             int64_t key, pagenum_t right_num, page_t* right);
void insert_into_leaf(int table_id, pagenum_t leaf_num, page_t* leaf, int64_t key, char* value);
pagenum_t insert_into_leaf_after_splitting(int table_id, pagenum_t root_num, pagenum_t leaf_num, page_t* leaf,
                                           int64_t key, char* value);
int db_insert(int table_id, int64_t key, char* value);

// delete
void remove_entry_from_leaf(page_t* node, int64_t key);
void remove_entry_from_node(page_t* node, int64_t key);
pagenum_t adjust_root(int table_id, pagenum_t root_num);
int get_neighbor_index(page_t* parent, pagenum_t node_num);
pagenum_t coalesce_nodes(int table_id, pagenum_t root_num, pagenum_t node_num, page_t* node,
                         pagenum_t neighbor_num, page_t* neighbor, int neighbor_index,
                         int64_t k_prime);
void redistribute_nodes(int table_id, pagenum_t node_num, page_t* node, pagenum_t neighbor_num, page_t* neighbor,
                        int neighbor_index, int k_prime_index, int64_t k_prime);
pagenum_t delete_entry(int table_id, pagenum_t root_num, pagenum_t node_num, int64_t key);
int db_delete(int table_id, int64_t key);

// print
void enqueue(Queue* q, pagenum_t data);
pagenum_t dequeue(Queue* q);
void print_tree(int table_id);

#endif
