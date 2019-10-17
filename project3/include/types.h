// page properties
#define PAGE_SIZE 4096
#define OFF(pagenum) ((pagenum) * PAGE_SIZE)
#define INTERNAL_ORDER 249
#define LEAF_ORDER 32
#define VALUE_SIZE 120

#define QUEUE_SIZE 1000

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

// extern variables
extern int fds[10];
extern buffer_t* buf_pool;

/* key, value type for leaf node
 */
struct key_value_t {
    int64_t key;
    char value[VALUE_SIZE];
};

/* key, page_number type for internal node
 */
struct key_page_t {
    int64_t key;
    pagenum_t page_number;
};

/* first page (offset 0-4095) of a data file
 * contains metadata.
 */
struct header_page_t {
    pagenum_t free_page_number;
    pagenum_t root_page_number;
    uint64_t number_of_pages;
    int8_t reserved[4072];
};

/* free page layout
 * linked and allocation is managed by the free page list.
 */
struct free_page_t {
    pagenum_t next_free_page_number;
    int8_t not_used[4088];
};

/* Internal/Leaf page
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

/* in-memory page structure
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
