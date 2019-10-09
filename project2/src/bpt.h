#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define OFF(pagenum) ((pagenum) * PAGE_SIZE)
#define INTERNAL_ORDER 249
#define LEAF_ORDER 32

#define BAD_REQUEST -400
#define NOT_FOUND -404
#define CONFLICT -409
#define INTERNAL_ERR -500

typedef uint64_t pagenum_t;

typedef struct key_value_t key_value_t;
typedef struct key_page_t key_page_t;
typedef struct header_page_t header_page_t;
typedef struct free_page_t free_page_t;
typedef struct node_page_t node_page_t;
typedef struct page_t page_t;

/* key, value type for leaf node */
struct key_value_t {
    int64_t key;
    char value[120];
};

/* key, page_number type for internal node */
struct key_page_t {
    int64_t key;
    uint64_t page_number;
};

struct header_page_t {
    uint64_t free_page_number;
    uint64_t root_page_number;
    uint64_t number_of_pages;
    int8_t reserved[4072];
};

struct free_page_t {
    uint64_t next_free_page_number;
    int8_t not_used[4088];
};

struct node_page_t {
    uint64_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    int8_t reserved[104];

    /* right_sibling_page_number for leaf node,
     * one_more_page_number for internal node
     */
    union {
        uint64_t right_sibling_page_number;
        uint64_t one_more_page_number;
    };

    /* key_values for leaf node,
     * key_page_numbers for internal node
     */
    union {
        key_value_t key_values[31];
        key_page_t key_page_numbers[248];
    };
};

struct page_t {
    union{
        header_page_t header;
        free_page_t free;
        node_page_t node;
    };
};

pagenum_t file_alloc_page();
void file_free_page(pagenum_t pagenum);
void file_read_page(pagenum_t pagenum, page_t* dest);
void file_write_page(pagenum_t pagenum, const page_t* src);

int open_table(char* pathname);

pagenum_t find_leaf(pagenum_t root, int64_t key);
int db_find(int64_t key, char* ret_val);

pagenum_t make_node(page_t* node);
pagenum_t start_new_tree(int64_t key, char* value);
pagenum_t insert_into_new_root(pagenum_t left_num, page_t* left, int key, pagenum_t right_num, page_t* right);
int get_left_index(page_t* parent, pagenum_t left_num);
void insert_into_node(page_t* parent, int left_index, int key, pagenum_t right_num);
pagenum_t insert_into_node_after_splitting(pagenum_t root_num, pagenum_t parent_num, page_t* parent, int left_index, int key, pagenum_t right_num);
pagenum_t insert_into_parent(pagenum_t root_num, pagenum_t left_num, page_t* left, int key, pagenum_t right_num, page_t* right);
int cut(int len);
void insert_into_leaf(pagenum_t leaf_num, page_t* leaf, int64_t key, char* value);
pagenum_t insert_into_leaf_after_splitting(pagenum_t root_num, pagenum_t leaf_num, page_t* leaf, int64_t key, char* value);
int db_insert(int64_t key, char* value);

int db_delete(int64_t key);

extern int fd;

#endif