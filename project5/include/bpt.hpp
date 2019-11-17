#ifndef __BPT_H__
#define __BPT_H__

#include "types.hpp"
#include "disk_space_manager.hpp"
#include "buffer_manager.hpp"
#include "printer.hpp"
#include "join.hpp"

/// open

/**
 * Open existing data file using ‘pathname’ or create one if not existed.
 * 
 * @param pathname pathname for opening file
 * @returns unique table id if success. Otherwise, return negative value
 */
int open_table(char* pathname);

/// util

/**
 * Finds the appropriate place to split a node that is too big into two.
 * 
 * @param len original length to cut
 * @returns rounded value of len
 */
int cut(int len);

/// find

/**
 * Traces the path from the root to a leaf, searching by key.
 * 
 * @param table_id 
 * @param root
 * @param key
 * @returns leaf page's number containing given key. if not found, return 0
 */
pagenum_t find_leaf(int table_id, pagenum_t root, int64_t key);

/**
 * Find the record containing input ‘key’.
 * 
 * @param table_id unique table id for search
 * @param key key to search
 * @param ret_val store matched value of given key in here
 * @returns 0 if success. Otherwise, return negative value
 */
int db_find(int table_id, int64_t key, char* ret_val);

// insert

/**
 * First insertion: start a new tree.
 * 
 * @param header pinned header buffer of current table
 * @param key inserting key
 * @param value inserting value
 * @returns new root's page number.
 */
pagenum_t start_new_tree(buffer_t* header, int64_t key, char* value);

/**
 * Creates a new root for two subtrees and inserts the appropriate key into the new root.
 * 
 * @param header pinned header buffer of current table
 * @param left pinned left node's buffer
 * @param key key to insert in new root
 * @param right pinned right node's buffer
 * @returns new root's page number
 */
pagenum_t insert_into_new_root(buffer_t* header, buffer_t* left, int64_t key, buffer_t* right);

/**
 * Helper function used in insert_into_parent to find the index of the parent's pointer
 * to the node to the left of the key to be inserted.
 * 
 * @param parent pinned parent node to find index from
 * @param left_num node's page number to find index
 * @returns index of left_num in parent's pointers
 */
int get_left_index(buffer_t* parent, pagenum_t left_num);

/**
 * Inserts a new key and pointer to a node into a node into which these can fit
 * without violating the B+ tree properties.
 * 
 * @param table_id table id of inserting node
 * @param parent pinned parent node to insert key
 * @param left_index left child's index in parent's pointers
 * @param key inserting key
 * @param right_num page number of left child's right sibling node
 */
void insert_into_node(int table_id, buffer_t* parent, int left_index, int64_t key, pagenum_t right_num);

/**
 * Inserts a new key and pointer to a node, causing the node's size to exceed the order
 * and causing the node to split into two.
 * 
 * @param header pinned header buffer of current table
 * @param root_num root number of current table
 * @param parent pinned parent node to insert key
 * @param left_index left child's index in parent's pointers
 * @param key inserting key
 * @param right_num page number of left child's right sibling node
 * @returns root page's page number
 */
pagenum_t insert_into_node_after_splitting(buffer_t* header, pagenum_t root_num, buffer_t* parent,
                                           int left_index, int64_t key, pagenum_t right_num);

/**
 * Inserts a new node (leaf or internal node) into the B+ tree.
 * 
 * @param header pinned header buffer of current table
 * @param root_num root number of current table
 * @param left pinned left child buffer
 * @param key inserting key
 * @param right pinned right child buffer
 * @returns the root page's page number after insertion
 */
pagenum_t insert_into_parent(buffer_t* header, pagenum_t root_num, buffer_t* left,
                             int64_t key, buffer_t* right);

/**
 * Inserts a new pointer to a record and its corresponding key into a leaf.
 * 
 * @param table_id unique table id of current table
 * @param leaf pinned leaf number to insert key
 * @param key inserting key
 * @param value inserting value
 */
void insert_into_leaf(int table_id, buffer_t* leaf, int64_t key, char* value);

/**
 * Inserts a new key and pointer to a new record into a leaf so as to exceed the tree's order,
 * causing the leaf to be split in half.
 * 
 * @param header pinned header page of current table
 * @param root_num root number of current table
 * @param leaf pinned leaf number to insert key
 * @param key inserting key
 * @param value inserting value
 * @returns new root's page number
 */
pagenum_t insert_into_leaf_after_splitting(buffer_t* header, pagenum_t root_num, buffer_t* leaf,
                                           int64_t key, char* value);

/**
 * Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 * 
 * @param table_id unique table id to insert record
 * @param key inserting key
 * @param value inserting value
 * @returns 0 if success, else return negative value
 */
int db_insert(int table_id, int64_t key, char* value);

// delete

/**
 * Delete single record from leaf node.
 * 
 * @param node leaf node to delete key
 * @param key deleting key
 */
void remove_entry_from_leaf(buffer_t* node, int64_t key);

/**
 * Delete single record from internal node.
 * 
 * @param node internal node to delete key
 * @param key deleting key
 */
void remove_entry_from_node(buffer_t* node, int64_t key);

/**
 * Changes root number if old root is empty.
 * 
 * @param header pinned header buffer of current table
 * @param root_num root number of current table
 * @returns new root's page number
 */
pagenum_t adjust_root(buffer_t* header, pagenum_t root_num);

/**
 * Utility function for deletion. Retrives left sibling of node.
 * 
 * @param parent pinned parent buffer of node
 * @param node_num searching node's page number
 * @returns node's left sibling's number if exists. Otherwise, return -1
 */
int get_neighbor_index(buffer_t* parent, pagenum_t node_num);

/**
 * Coalesces a node that has become too small after deletion
 * with a neighboring node that can accept the additional entries
 * without exceeding the maximum.
 * 
 * @param header pinned header buffer of current table
 * @param root_num root number of current table
 * @param node pinned node to coalesce
 * @param neighbor pinned neighbor node to coalesce
 * @param neighbor_index result of function get_neighbor_index
 * @param k_prime deleting key from parent after coalescing
 * @returns root page's page number
 */
pagenum_t coalesce_nodes(buffer_t* header, pagenum_t root_num, buffer_t* node,
                         buffer_t* neighbor, int neighbor_index, int64_t k_prime);

/**
 * Redistributes entries between two nodes when one has become too small after deletion
 * but its neighbor is too big to append the small node's entries without exceeding the maximum.
 * only called in internal node.
 * 
 * @param table_id unique table id of current table
 * @param node pinned node to redistribute
 * @param neighbor pinned neighbor node to redistribute
 * @param neighbor_index result of function get_neighbor_index
 * @param k_prime_index index of k_prime in parent node's pointers
 * @param k_prime deleting key from parent
 */
void redistribute_nodes(int table_id, buffer_t* node, buffer_t* neighbor,
                        int neighbor_index, int k_prime_index, int64_t k_prime);

/**
 * Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer from the leaf,
 * and then makes all appropriate changes to preserve the B+ tree properties.
 * 
 * @param header pinned header buffer of current table
 * @param root_num root number of current table
 * @param node_num node number to delete record from
 * @param key deleting key
 * @returns root page's page number after deletion.
 */
pagenum_t delete_entry(buffer_t* header, pagenum_t root_num, pagenum_t node_num, int64_t key);

/**
 * Find the matching record and delete it if found.
 * 
 * @param table_id unique table id of current table
 * @param key deleting key
 * @returns 0 if success. Otherwise, return negative value
 */
int db_delete(int table_id, int64_t key);

#endif
