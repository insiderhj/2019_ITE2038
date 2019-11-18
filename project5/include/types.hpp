#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include <list>

// page properties
#define PAGE_SIZE 4096
#define OFF(pagenum) ((pagenum) * PAGE_SIZE)

#define INTERNAL_ORDER 249
#define LEAF_ORDER 32

// #define INTERNAL_ORDER 5
// #define LEAF_ORDER 4

#define VALUE_SIZE 120

#define QUEUE_SIZE 200000

// err numbers
#define BAD_REQUEST -400
#define NOT_FOUND -404
#define CONFLICT -409
#define INTERNAL_ERR -500

// type definitions
typedef uint64_t pagenum_t;
typedef struct key_value_t key_value_t;
typedef struct key_page_t key_page_t;
typedef struct header_page_t header_page_t;
typedef struct free_page_t free_page_t;
typedef struct node_page_t node_page_t;
typedef struct page_t page_t;

typedef struct Queue Queue;

typedef struct buffer_t buffer_t;
typedef struct buffer_pool_t buffer_pool_t;

typedef struct lock_t lock_t;
typedef struct trx_t trx_t;

// extern variables
extern int fds[10];
extern buffer_pool_t buf_pool;
extern int init;
extern char pathnames[10][512];
extern std::list<trx_t> trxs;
extern int max_tid;

enum lock_mode {
    SHARED,
    EXCLUSIVE,
};

enum trx_state {
    IDLE,
    RUNNING,
    WAITING,
};

/**
 * key, value type for leaf node
 */
struct key_value_t {
    int64_t key;
    char value[VALUE_SIZE];
};

/**
 * key, page_number type for internal node
 */
struct key_page_t {
    int64_t key;
    pagenum_t page_number;
};

/**
 * first page (offset 0-4095) of a data file
 * contains metadata.
 */
struct header_page_t {
    pagenum_t free_page_number;
    pagenum_t root_page_number;
    uint64_t number_of_pages;
    int8_t reserved[4072];
};

/**
 * free page layout
 * linked and allocation is managed by the free page list.
 */
struct free_page_t {
    pagenum_t next_free_page_number;
    int8_t not_used[4088];
};

/**
 * Internal/Leaf page
 */
struct node_page_t {
    pagenum_t parent_page_number;
    uint32_t is_leaf;
    uint32_t number_of_keys;
    int8_t reserved[104];

    /* right_sibling_page_number for leaf node,
     * one_more_page_number for internal node
     */
    union {
        pagenum_t right_sibling_page_number;
        pagenum_t one_more_page_number;
    };

    /* key_values for leaf node,
     * key_page_numbers for internal node
     */
    union {
        key_value_t key_values[31];
        key_page_t key_page_numbers[248];
    };
};

/**
 * in-memory page structure
 */
struct page_t {
    union{
        header_page_t header;
        free_page_t free;
        node_page_t node;
    };
};

struct Queue {
    int front, rear;
    int item_count;
    pagenum_t pages[QUEUE_SIZE];
};

struct buffer_t {
    page_t frame;
    int table_id;
    pagenum_t page_num;
    uint32_t is_dirty;
    uint32_t is_pinned;
    uint32_t is_allocated;
    int next;
    int prev;
};

struct buffer_pool_t {
    buffer_t* buffers;
    int capacity;
    int num_buffers;
    int mru;
    int lru;
};

struct lock_t {
    int table_id;
    int key;
    lock_mode mode;
    trx_t* trx;
};

struct trx_t {
    int trx_id;
    trx_state state;
    std::list<lock_t*> trx_locks;
    lock_t* wait_lock;
};

#endif
