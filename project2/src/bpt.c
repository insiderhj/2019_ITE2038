#include "bpt.h"

int fd = 0;
page_t header_page;

/* Allocate an on-disk page from the free page list
 */
pagenum_t file_alloc_page() {
    file_read_page(0, &header_page);

    // case: no free page
    if (header_page.header.free_page_number == 0) {
        // create new page and set all to 0
        page_t new_page;
        int64_t new_page_number = header_page.header.number_of_pages;
        memset(&new_page, 0, PAGE_SIZE);

        // modify free page list
        new_page.free.next_free_page_number = header_page.header.free_page_number;
        header_page.header.free_page_number = new_page_number;
        header_page.header.number_of_pages++;

        // write changes to file
        file_write_page(0, &header_page);
        file_write_page(new_page_number, &new_page);
    }
    
    // get one free page from list
    page_t buf;
    int64_t free_page_number = header_page.header.free_page_number;
    file_read_page(free_page_number, &buf);

    // modify free page list
    header_page.header.free_page_number = buf.free.next_free_page_number;
    file_write_page(0, &header_page);

    return free_page_number;
}

/* Free an on-disk page to the free page list
 */
void file_free_page(pagenum_t pagenum) {
    file_read_page(0, &header_page);

    page_t p;
    memset(&p, 0, PAGE_SIZE);

    // make new free page, then modify free page list
    uint64_t first_free_page = header_page.header.free_page_number;
    p.free.next_free_page_number = first_free_page;
    header_page.header.free_page_number = pagenum;

    // write to disk
    file_write_page(0, &header_page);
    file_write_page(pagenum, &p);
}

/* Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(pagenum_t pagenum, page_t* dest) {
    pread(fd, dest, PAGE_SIZE, OFF(pagenum));
}

/* Write an in-memory page(src) to the on-disk page
 */
void file_write_page(pagenum_t pagenum, const page_t* src) {
    pwrite(fd, src, PAGE_SIZE, OFF(pagenum));
}

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return the unique table id, which represents the own table in this database.
 * Otherwise, return negative value. (This table id will be used for future assignment.)
 */
int open_table(char* pathname) {
    // pathname is null
    if (!pathname) return BAD_REQUEST;

    // fd already opened
    if (fd) return CONFLICT;

    int fd = open(pathname, O_CREAT | O_NOFOLLOW | O_RDWR | O_SYNC);

    // open db failed
    if (fd == -1) return INTERNAL_ERR;

    if (read(fd, &header_page, PAGE_SIZE) == 0) {
        memset(&header_page, 0, PAGE_SIZE);
        header_page.header.number_of_pages = 1;
        file_write_page(0, &header_page);
    }

    return fd;
}

/* Traces the path from the root to a leaf, searching by key
 * Returns the leaf page's page number containing the given key.
 */
pagenum_t find_leaf(pagenum_t root, int64_t key) {
    int i;
    pagenum_t c_num = root;
    page_t c;
    file_read_page(c_num, &c);

    // tree doesn't have root
    if (c_num == 0) return c_num;

    while (!c.node.is_leaf) {
        i = 0;
        while (c.node.number_of_keys) {
            if (key >= c.node.key_page_numbers[i].key) i++;
            else break;
        }
        c_num = c.node.key_page_numbers[i].page_number;
        file_read_page(c.node.key_page_numbers[i].page_number, &c);
    }
    return c_num;
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’,
 * store matched ‘value’ string in ret_val and return 0.
 * Otherwise, return non-zero value.
 */
int db_find(int64_t key, char* ret_val) {
    file_read_page(0, &header_page);
    int i = 0;
    pagenum_t leaf_num = find_leaf(header_page.header.root_page_number, key);
    page_t leaf;

    if (leaf_num == 0) return NOT_FOUND;

    file_read_page(leaf_num, &leaf);
    
    for (i = 0; i < leaf.node.number_of_keys; i++) {
        if (leaf.node.key_values[i].key == key) break;
    }
    
    if (i == leaf.node.number_of_keys) return NOT_FOUND;
    
    strcpy(ret_val, leaf.node.key_values[i].value);
    return 0;
}

/* Creates a new node
 */
pagenum_t make_node(page_t* node) {
    pagenum_t node_num = file_alloc_page(node);
    memset(node, 0, PAGE_SIZE);
    return node_num;
}

/* First insertion:
 * start a new tree.
 */
int start_new_tree(int64_t key, char* value) {
    page_t root;
    pagenum_t root_number = make_node(&root);
    root.node.is_leaf = true;
    root.node.key_values[0].key = key;
    strcpy(root.node.key_values[0].value, value);
    root.node.number_of_keys++;
    
    header_page.header.root_page_number = root_number;
    
    file_write_page(0, &header_page);
    file_write_page(root_number, &root);
    return 0;
}

/* Inserts a new pointer to a record and its corresponding key into a leaf.
 */
void insert_into_leaf(pagenum_t leaf_num, page_t* leaf, int64_t key, char* value) {
    int i, insertion_point = 0;

    while (insertion_point < leaf->node.number_of_keys
           && leaf->node.key_values[insertion_point].key < key) 
        insertion_point++;
    
    for (i = leaf->node.number_of_keys; i > insertion_point; i--) {
        leaf->node.key_values[i].key = leaf->node.key_values[i - 1].key;
        strcpy(leaf->node.key_values[i].value, leaf->node.key_values[i - 1].value);
    }
    leaf->node.key_values[insertion_point].key = key;
    strcpy(leaf->node.key_values[insertion_point].value, value);
    leaf->node.number_of_keys++;
    file_write_page(leaf_num, &leaf);
}

int cut(int len) {
    return len % 2 == 0 ? len / 2 : len / 2 + 1;
}

// TODO
/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(pagenum_t root_num, page_t* leaf, int64_t key, char* value) {
    page_t new_leaf;
    pagenum_t new_leaf_num;
    int temp_keys[LEAF_ORDER];
    uint8_t temp_values[120][LEAF_ORDER];
    int insertion_index, split, new_key, i, j;

    new_leaf_num = make_node(&new_leaf);
    new_leaf.node.is_leaf = true;

    insertion_index = 0;
    while (insertion_index < LEAF_ORDER - 1 && leaf->node.key_values[insertion_index].key) {
            insertion_index++;
    }
    for (i = 0, j = 0; i < leaf->node.number_of_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf->node.key_values[i].key;
        strcpy(temp_values[j], leaf->node.key_values[i].value);
    }

    temp_keys[insertion_index] = key;
    strcpy(temp_values[insertion_index], value);

    split = cut(LEAF_ORDER - 1);

    for(i = 0; i < split; i++) {
        strcpy(leaf->node.key_values[i].value, temp_values[i]);
        leaf->node.key_values[i].key = temp_keys[i];
    }
    leaf->node.number_of_keys = split;

    for (i = split, j = 0; i < LEAF_ORDER; i++, j++) {
        strcpy(new_leaf.node.key_values[j].value, temp_values[i]);
        new_leaf.node.key_values[j].key = temp_keys[i];
    }
    new_leaf.node.number_of_keys = LEAF_ORDER - split;

    new_leaf.node.parent_page_number = leaf->node.parent_page_number;
    new_key = new_leaf.node.key_values[0].key;

    return insert_into_parent(root_num, leaf, new_key, new_leaf);
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int db_insert(int64_t key, char* value) {
    uint8_t res[120];
    
    if (fd == 0) return BAD_REQUEST;
    if (db_find(key, res) == 0) return CONFLICT;

    if (header_page.header.root_page_number == 0) return start_new_tree(key, value);

    pagenum_t leaf_num = find_leaf(header_page.header.root_page_number, key);
    page_t leaf;
    file_read_page(leaf_num, &leaf);
    if (leaf.node.number_of_keys < LEAF_ORDER) {
        insert_into_leaf(leaf_num, &leaf, key, value);
        return 0;
    }

    insert_into_leaf_after_splitting(header_page.header.root_page_number, &leaf, key, value);
    return 0;
}

int db_delete(int64_t key) {

}
