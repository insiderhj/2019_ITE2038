#include "bpt.h"

char* pathnames[10];
int fds[10];
int init;

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return the unique table id, which represents the own table in this database.
 * Otherwise, return negative value. (This table id will be used for future assignment.)
 */
int open_table(char* pathname) {
    // pathname is null
    if (pathname == NULL) return BAD_REQUEST;

    // file array is full
    if (fds[9]) return CONFLICT;

    // didn't initialize db
    if (!init) return BAD_REQUEST;

    // file already opened
    if (check_pathname(pathname) == CONFLICT) return CONFLICT;

    int i = -1;
    while (fds[++i]);
    fds[i] = buf_read_table(pathname);
    pathnames[i] = pathname;

    // open db failed
    if (fds[i] == -1) {
        fds[i] = 0;
        return INTERNAL_ERR;
    }
    return fds[i];
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut(int len) {
    return len % 2 == 0 ? len / 2 : len / 2 + 1;
}

/* Traces the path from the root to a leaf, searching by key
 * Returns the leaf page's page number containing the given key.
 */
pagenum_t find_leaf(int table_id, pagenum_t root_num, int64_t key) {
    int i;
    pagenum_t c_num = root_num;

    // tree doesn't have root
    if (c_num == 0) return c_num;

    buffer_t* c;
    // 001
    c = get_buf(table_id, c_num, 0);

    while (!c->frame.node.is_leaf) {
        i = 0;
        while (i < c->frame.node.number_of_keys) {
            if (key >= c->frame.node.key_page_numbers[i].key) i++;
            else break;
        }
        if (i == 0) c_num = c->frame.node.one_more_page_number;
        else {
            c_num = c->frame.node.key_page_numbers[i - 1].page_number;
        }
        unpin(c);
        c = get_buf(table_id, c_num, 0);
    }
    // 001
    unpin(c);
    return c_num;
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’,
 * store matched ‘value’ string in ret_val and return 0.
 * Otherwise, return non-zero value.
 */
int db_find(int table_id, int64_t key, char* ret_val) {
    buffer_t* header_page, * leaf;
    // 002
    header_page = get_buf(table_id, 0, 0);
    if (header_page->frame.header.root_page_number == 0) {
        unpin(header_page);
        return NOT_FOUND;
    }

    pagenum_t leaf_num = find_leaf(table_id, header_page->frame.header.root_page_number, key);
    // 002
    unpin(header_page);

    // leaf not found
    if (leaf_num == 0) return NOT_FOUND;

    // 003
    leaf = get_buf(table_id, leaf_num, 0);

    // search key in the leaf
    int i;
    for (i = 0; i < leaf->frame.node.number_of_keys; i++) {
        if (leaf->frame.node.key_values[i].key == key) break;
    }
    
    // value not found
    if (i == leaf->frame.node.number_of_keys) {
        // 003
        unpin(leaf);
        return NOT_FOUND;
    }
    
    // copy value into ret_val
    strcpy(ret_val, leaf->frame.node.key_values[i].value);
    // 003
    unpin(leaf);
    return 0;
}

buffer_t* make_node(buffer_t* header) {
    buffer_t* node = buf_alloc_page(header);
    memset(node, 0, PAGE_SIZE);
    return node;
}

/* First insertion:
 * start a new tree.
 * returns new root's page number.
 */
pagenum_t start_new_tree(buffer_t* header, int64_t key, char* value) {
    buffer_t* root = make_node(header);
    root->frame.node.is_leaf = true;
    root->frame.node.key_values[0].key = key;
    strcpy(root->frame.node.key_values[0].value, value);
    root->frame.node.number_of_keys++;

    // 004
    unpin(root);
    return root->page_num;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 * returns new root's page number
 */
pagenum_t insert_into_new_root(buffer_t* header, buffer_t* left,
                               int64_t key, buffer_t* right) {
    buffer_t* root = make_node(header);
    root->frame.node.key_page_numbers[0].key = key;
    root->frame.node.one_more_page_number = left->page_num;
    root->frame.node.key_page_numbers[0].page_number = right->page_num;
    root->frame.node.number_of_keys++;

    // set parent numbers
    left->frame.node.parent_page_number = root->page_num;
    right->frame.node.parent_page_number = root->page_num;

    // 004
    unpin(root);
    return root->page_num;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the node to the left of the key to be inserted.
 */
int get_left_index(buffer_t* parent, pagenum_t left_num) {
    if (parent->frame.node.one_more_page_number == left_num) return 0;

    int left_index = 1;
    while (left_index <= parent->frame.node.number_of_keys
           && parent->frame.node.key_page_numbers[left_index - 1].page_number != left_num)
        left_index++;
    
    return left_index;
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(int table_id, buffer_t* parent, int left_index, int64_t key, pagenum_t right_num) {
    int i;
    for (i = parent->frame.node.number_of_keys; i > left_index; i--) {
        parent->frame.node.key_page_numbers[i].page_number =
            parent->frame.node.key_page_numbers[i - 1].page_number;
        parent->frame.node.key_page_numbers[i].key = parent->frame.node.key_page_numbers[i - 1].key;
    }
    parent->frame.node.key_page_numbers[left_index].page_number = right_num;
    parent->frame.node.key_page_numbers[left_index].key = key;
    parent->frame.node.number_of_keys++;
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 * returns root page's page number
 */
pagenum_t insert_into_node_after_splitting(buffer_t* header, pagenum_t root_num,
                                           buffer_t* parent,
                                           int left_index, int64_t key, pagenum_t right_num) {
    int i, j, split;
    int64_t k_prime;
    buffer_t* new_parent = make_node(header), * child;
    int64_t temp_keys[249];
    pagenum_t temp_page_numbers[250], new_root_num;

    // copy page numbers into temp arr
    temp_page_numbers[0] = parent->frame.node.one_more_page_number;
    for (i = 0, j = left_index == 0 ? 2 : 1; i < parent->frame.node.number_of_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp_page_numbers[j] = parent->frame.node.key_page_numbers[i].page_number;
    }

    for (i = 0, j = 0; i < parent->frame.node.number_of_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = parent->frame.node.key_page_numbers[i].key;
    }

    // insert new key and page number
    temp_page_numbers[left_index + 1] = right_num;
    temp_keys[left_index] = key;

    split = cut(INTERNAL_ORDER);
    
    // copy from temp arr to old parent
    parent->frame.node.one_more_page_number = temp_page_numbers[0];
    for (i = 0; i < split - 1; i++) {
        parent->frame.node.key_page_numbers[i].page_number = temp_page_numbers[i + 1];
        parent->frame.node.key_page_numbers[i].key = temp_keys[i];
    }
    parent->frame.node.number_of_keys = split - 1;
    k_prime = temp_keys[split - 1];

    // copy from temp arr to new parent
    new_parent->frame.node.one_more_page_number = temp_page_numbers[++i];
    for (j = 0; i < INTERNAL_ORDER; i++, j++) {
        new_parent->frame.node.key_page_numbers[j].page_number = temp_page_numbers[i + 1];
        new_parent->frame.node.key_page_numbers[j].key = temp_keys[i];
    }
    new_parent->frame.node.number_of_keys = INTERNAL_ORDER - split;
    new_parent->frame.node.parent_page_number = parent->frame.node.parent_page_number;

    // set parent node number of first child
    // 005
    child = get_buf(header->table_id, new_parent->frame.node.one_more_page_number, 1);
    child->frame.node.parent_page_number = new_parent->page_num;
    // 005
    unpin(child);

    // set parent node number of other children
    for (i = 0; i < new_parent->frame.node.number_of_keys; i++) {
        // 006
        child = get_buf(header->table_id, new_parent->frame.node.key_page_numbers[i].page_number, 1);
        child->frame.node.parent_page_number = new_parent->page_num;
        // 006
        unpin(child);
    }

    new_root_num = insert_into_parent(header, root_num, parent, k_prime, new_parent);
    unpin(new_parent);
    return new_root_num;
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root page's page number after insertion.
 */
pagenum_t insert_into_parent(buffer_t* header, pagenum_t root_num, buffer_t* left,
                             int64_t key, buffer_t* right) {
    int left_index;
    pagenum_t new_root_num;

    // case: left is root
    if (left->frame.node.parent_page_number == 0) {
        return insert_into_new_root(header, left, key, right);
    }

    buffer_t* parent;
    // 007
    parent = get_buf(header->table_id, left->frame.node.parent_page_number, 1);
    left_index = get_left_index(parent, left->page_num);

    if (parent->frame.node.number_of_keys < INTERNAL_ORDER - 1) {
        insert_into_node(header->table_id, parent, left_index, key, right->page_num);
        return root_num;
    }

    new_root_num = insert_into_node_after_splitting(header, root_num, parent,
                                            left_index, key, right->page_num);
    // 007
    unpin(parent);
    return new_root_num;
}

/* Inserts a new pointer to a record and its corresponding key into a leaf.
 */
void insert_into_leaf(int table_id, buffer_t* leaf, int64_t key, char* value) {
    int i, insertion_point = 0;

    while (insertion_point < leaf->frame.node.number_of_keys
           && leaf->frame.node.key_values[insertion_point].key < key) 
        insertion_point++;
    
    for (i = leaf->frame.node.number_of_keys; i > insertion_point; i--) {
        leaf->frame.node.key_values[i].key = leaf->frame.node.key_values[i - 1].key;
        strcpy(leaf->frame.node.key_values[i].value, leaf->frame.node.key_values[i - 1].value);
    }
    leaf->frame.node.key_values[insertion_point].key = key;
    strcpy(leaf->frame.node.key_values[insertion_point].value, value);
    leaf->frame.node.number_of_keys++;
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split in half.
 */
pagenum_t insert_into_leaf_after_splitting(buffer_t* header, pagenum_t root_num, buffer_t* leaf,
                                      int64_t key, char* value) {
    buffer_t* new_leaf = make_node(header);
    int temp_keys[LEAF_ORDER];
    char temp_values[LEAF_ORDER][VALUE_SIZE];
    int insertion_index, split, new_key, i, j;
    pagenum_t new_root_num;

    new_leaf->frame.node.is_leaf = true;

    insertion_index = 0;
    while (insertion_index < LEAF_ORDER - 1 && leaf->frame.node.key_values[insertion_index].key < key) {
            insertion_index++;
    }
    for (i = 0, j = 0; i < leaf->frame.node.number_of_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf->frame.node.key_values[i].key;
        strcpy(temp_values[j], leaf->frame.node.key_values[i].value);
    }

    temp_keys[insertion_index] = key;
    strcpy(temp_values[insertion_index], value);

    split = cut(LEAF_ORDER - 1);

    for(i = 0; i < split; i++) {
        strcpy(leaf->frame.node.key_values[i].value, temp_values[i]);
        leaf->frame.node.key_values[i].key = temp_keys[i];
    }
    leaf->frame.node.number_of_keys = split;

    for (i = split, j = 0; i < LEAF_ORDER; i++, j++) {
        strcpy(new_leaf->frame.node.key_values[j].value, temp_values[i]);
        new_leaf->frame.node.key_values[j].key = temp_keys[i];
    }
    new_leaf->frame.node.number_of_keys = LEAF_ORDER - split;

    new_leaf->frame.node.right_sibling_page_number = leaf->frame.node.right_sibling_page_number;
    leaf->frame.node.right_sibling_page_number = new_leaf->page_num;

    new_leaf->frame.node.parent_page_number = leaf->frame.node.parent_page_number;
    new_key = new_leaf->frame.node.key_values[0].key;

    new_root_num = insert_into_parent(header, root_num, leaf, new_key, new_leaf);
    unpin(new_leaf);
    return new_root_num;
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int db_insert(int table_id, int64_t key, char* value) {
    if (table_id == 0) return BAD_REQUEST;

    char res[VALUE_SIZE];
    if (db_find(table_id, key, res) == 0) return CONFLICT;

    buffer_t* header_page;
    pagenum_t root_num;
    // 008
    header_page = get_buf(table_id, 0, 1);
    
    // case: file has no root page
    if (header_page->frame.header.root_page_number == 0) {
        root_num = start_new_tree(header_page, key, value);
        set_root(header_page, root_num);
        // 008
        unpin(header_page);
        return 0;
    }

    pagenum_t leaf_num = find_leaf(table_id, header_page->frame.header.root_page_number, key);
    buffer_t* leaf;
    // 009
    leaf = get_buf(table_id, leaf_num, 1);
    if (leaf->frame.node.number_of_keys < LEAF_ORDER - 1) {
        insert_into_leaf(table_id, leaf, key, value);
        // 008
        unpin(header_page);
        // 009
        unpin(leaf);
        return 0;
    }

    root_num = insert_into_leaf_after_splitting(
        header_page, header_page->frame.header.root_page_number, leaf, key, value);
    set_root(header_page, root_num);
    // 008
    unpin(header_page);
    // 009
    unpin(leaf);
    return 0;
}

void remove_entry_from_leaf(buffer_t* node, int64_t key) {
    int i;
    i = 0;

    while (node->frame.node.key_values[i].key != key) i++;
    for (++i; i < node->frame.node.number_of_keys; i++) {
        node->frame.node.key_values[i - 1].key = node->frame.node.key_values[i].key;
        strcpy(node->frame.node.key_values[i - 1].value, node->frame.node.key_values[i].value);
    }
    node->frame.node.number_of_keys--;
}

void remove_entry_from_node(buffer_t* node, int64_t key) {
    int i;
    i = 0;

    while (node->frame.node.key_page_numbers[i].key != key) i++;
    for (++i; i < node->frame.node.number_of_keys; i++) {
        node->frame.node.key_page_numbers[i - 1].key = node->frame.node.key_page_numbers[i].key;
        node->frame.node.key_page_numbers[i - 1].page_number =
            node->frame.node.key_page_numbers[i].page_number;
    }
    node->frame.node.number_of_keys--;
}

/* returns root page's page number
 */
pagenum_t adjust_root(buffer_t* header_page, pagenum_t root_num) {
    pagenum_t new_root_num;
    buffer_t* root, * new_root;
    // 010
    root = get_buf(header_page->table_id, root_num, 1);

    // case: nonempty root
    if (root->frame.node.number_of_keys > 0) return root_num;
    
    // case: empty root
    if (root->frame.node.is_leaf) {
        new_root_num = 0;
    }
    else {
        new_root_num = root->frame.node.one_more_page_number;
        // 011
        new_root = get_buf(header_page->table_id, new_root_num, 1);
        new_root->frame.node.parent_page_number = 0;
        // 011
        unpin(new_root);
    }
    buf_free_page(header_page, root);
    // 010
    unpin(root);
    
    return new_root_num;
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(buffer_t* parent, pagenum_t node_num) {
    int i;
    if (parent->frame.node.one_more_page_number == node_num) return -1; 
    for (i = 0; i <= parent->frame.node.number_of_keys; i++) {
        if (parent->frame.node.key_page_numbers[i].page_number == node_num)
            return i;
    }
    return NOT_FOUND;
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 * returns root page's page number.
 */
pagenum_t coalesce_nodes(buffer_t* header_page, pagenum_t root_num, buffer_t* node,
                         buffer_t* neighbor,
                         int neighbor_index, int64_t k_prime) {
    int i, j, neighbor_insertion_index, node_end;
    pagenum_t tmp_num;
    buffer_t* tmp;

    if (neighbor_index == -1) {
        tmp = node;
        node = neighbor;
        neighbor = tmp;
    }
    
    neighbor_insertion_index = neighbor->frame.node.number_of_keys;

    // case: internal node
    if (!node->frame.node.is_leaf) {
        neighbor->frame.node.key_page_numbers[neighbor_insertion_index].key = k_prime;
        neighbor->frame.node.number_of_keys++;

        node_end = node->frame.node.number_of_keys;

        neighbor->frame.node.key_page_numbers[neighbor_insertion_index].page_number =
            node->frame.node.one_more_page_number;
        for (i = neighbor_insertion_index + 1, j = 0; j < node_end; i++, j++) {
            neighbor->frame.node.key_page_numbers[i].key = node->frame.node.key_page_numbers[j].key;
            neighbor->frame.node.key_page_numbers[i].page_number =
                node->frame.node.key_page_numbers[j].page_number;
        }
        neighbor->frame.node.number_of_keys += node_end;

        for (i = neighbor_insertion_index; i < neighbor->frame.node.number_of_keys; i++) {
            // 012
            tmp = get_buf(header_page->table_id, neighbor->frame.node.key_page_numbers[i].page_number, 1);
            tmp->frame.node.parent_page_number = neighbor->page_num;
            // 012
            unpin(tmp);
        }
    }

    // case: leaf node
    else {
        for (i = neighbor_insertion_index, j = 0; j < node->frame.node.number_of_keys; i++, j++) {
            neighbor->frame.node.key_values[i].key = node->frame.node.key_values[j].key;
            strcpy(neighbor->frame.node.key_values[i].value, node->frame.node.key_values[j].value);
        }
        neighbor->frame.node.number_of_keys += node->frame.node.number_of_keys;
        neighbor->frame.node.right_sibling_page_number = node->frame.node.right_sibling_page_number;
    }

    root_num = delete_entry(header_page, root_num, node->frame.node.parent_page_number, k_prime);
    buf_free_page(header_page, node);
    return root_num;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the maximum
 * only called in internal node
 */
void redistribute_nodes(int table_id, buffer_t* node, buffer_t* neighbor,
                        int neighbor_index, int k_prime_index, int64_t k_prime) {
    int i;
    buffer_t* parent, * tmp;
    // 013
    parent = get_buf(table_id, node->frame.node.parent_page_number, 1);

    // case: node is the leftmost child
    if (neighbor_index == -1) {
        node->frame.node.key_page_numbers[node->frame.node.number_of_keys].key = k_prime;
        node->frame.node.key_page_numbers[node->frame.node.number_of_keys].page_number =
            neighbor->frame.node.one_more_page_number;

        // 014
        tmp = get_buf(table_id, node->frame.node.key_page_numbers[node->frame.node.number_of_keys].page_number, 1);
        tmp->frame.node.parent_page_number = node->page_num;
        // 014
        unpin(tmp);

        parent->frame.node.key_page_numbers[k_prime_index].key =
            neighbor->frame.node.key_page_numbers[0].key;
        
        neighbor->frame.node.one_more_page_number = neighbor->frame.node.key_page_numbers[0].page_number;
        for (i = 0; i < neighbor->frame.node.number_of_keys - 1; i++) {
            neighbor->frame.node.key_page_numbers[i].key =
                neighbor->frame.node.key_page_numbers[i + 1].key;
            neighbor->frame.node.key_page_numbers[i].page_number =
                neighbor->frame.node.key_page_numbers[i + 1].page_number;
        }
    }

    // case: n has a neighbor to the left
    else {
        for (i = node->frame.node.number_of_keys; i > 0; i--) {
            node->frame.node.key_page_numbers[i].key = node->frame.node.key_page_numbers[i - 1].key;
            node->frame.node.key_page_numbers[i].page_number =
                node->frame.node.key_page_numbers[i - 1].page_number;
        }
        node->frame.node.key_page_numbers[0].page_number = node->frame.node.one_more_page_number;

        node->frame.node.one_more_page_number =
            neighbor->frame.node.key_page_numbers[neighbor->frame.node.number_of_keys - 1].page_number;
        
        // 015
        tmp = get_buf(table_id, node->frame.node.one_more_page_number, 1);
        tmp->frame.node.parent_page_number = node->page_num;
        // 015
        unpin(tmp);
        
        node->frame.node.key_page_numbers[0].key = k_prime;
        parent->frame.node.key_page_numbers[k_prime_index].key =
            neighbor->frame.node.key_page_numbers[neighbor->frame.node.number_of_keys - 1].key;
    }
    // 013
    unpin(parent);
    node->frame.node.number_of_keys++;
    neighbor->frame.node.number_of_keys--;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 * returns root page's page number after deletion.
 */
pagenum_t delete_entry(buffer_t* header_page, pagenum_t root_num, pagenum_t node_num, int64_t key) {
    int min_keys;
    pagenum_t neighbor_num, parent_num, new_root_num;
    int neighbor_index;
    int k_prime_index;
    int64_t k_prime;
    int capacity;
    buffer_t* node, * neighbor, * parent;
    // 017
    node = get_buf(header_page->table_id, node_num, 1);
    
    if (node->frame.node.is_leaf) remove_entry_from_leaf(node, key);
    else remove_entry_from_node(node, key);

    // case: node is root
    if (root_num == node_num) {
        // 017
        unpin(node);
        return adjust_root(header_page, root_num);
    }

    // min_keys = node.node.is_leaf ? cut(LEAF_ORDER - 1) : cut(INTERNAL_ORDER) - 1;
    min_keys = 1;

    // case: number of keys is greater than or equal to min_keys
    if (node->frame.node.number_of_keys >= min_keys) {
        // 017
        unpin(node);
        return root_num;
    }

    // case: number of keys is lower than min_keys
    // 018
    parent = get_buf(header_page->table_id, node->frame.node.parent_page_number, 1);
    
    neighbor_index = get_neighbor_index(parent, node_num);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = parent->frame.node.key_page_numbers[k_prime_index].key;
    switch (neighbor_index) {
        case -1:
            neighbor_num = parent->frame.node.key_page_numbers[0].page_number;
            break;
        case 0:
            neighbor_num = parent->frame.node.one_more_page_number;
            break;
        default:
            neighbor_num = parent->frame.node.key_page_numbers[neighbor_index - 1].page_number;
            break;
    }
    // 018
    unpin(parent);

    // 019
    neighbor = get_buf(header_page->table_id, neighbor_num, 1);
    capacity = node->frame.node.is_leaf ? LEAF_ORDER : INTERNAL_ORDER - 1;
    
    if (neighbor->frame.node.number_of_keys + node->frame.node.number_of_keys < capacity) {
        new_root_num = coalesce_nodes(header_page, root_num, node, neighbor,
                              neighbor_index, k_prime);
        // 017
        unpin(node);
        // 019
        unpin(neighbor);
        return new_root_num;
    }
    
    else {
        redistribute_nodes(header_page->table_id, node, neighbor, neighbor_index,
                                  k_prime_index, k_prime);
        // 017
        unpin(node);
        // 019
        unpin(neighbor);
        return root_num;
    }
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int db_delete(int table_id, int64_t key) {
    char res[VALUE_SIZE];
    pagenum_t leaf_num;

    // no root or no such key
    if (leaf_num == 0 || db_find(table_id, key, res) == NOT_FOUND) return NOT_FOUND;

    buffer_t* header_page;
    // 020
    header_page = get_buf(table_id, 0, 1);
    leaf_num = find_leaf(table_id, header_page->frame.header.root_page_number, key);

    header_page->frame.header.root_page_number = delete_entry(header_page, header_page->frame.header.root_page_number,
                                                        leaf_num, key);
    // 020
    unpin(header_page);
    return 0;
}

void enqueue(Queue* q, pagenum_t data) {
    if(q->item_count != QUEUE_SIZE) {
        if(q->rear == QUEUE_SIZE - 1) {
            q->rear = -1;            
        }       

      q->pages[++q->rear] = data;
      q->item_count++;
   }
}

pagenum_t dequeue(Queue* q) {
    int data = q->pages[q->front++];
	
    if(q->front == QUEUE_SIZE) {
        q->front = 0;
    }
	
    q->item_count--;
    return data;  
}

void print_tree(int table_id) {
    Queue q;
    int i;
    pagenum_t tmp_num;
    buffer_t* header_page, * tmp;
    int64_t enter_key;

    q.front = 0;
    q.rear = -1;
    q.item_count = 0;

    // 021
    header_page = get_buf(table_id, 0, 0);
    tmp_num = header_page->frame.header.root_page_number;

    // 021
    unpin(header_page);
    
    // case: no root
    if (tmp_num == 0) {
        printf("empty tree\n");
        return;
    }

    // enqueue root number
    enqueue(&q, tmp_num);

    while (q.item_count != 0) {
        tmp_num = dequeue(&q);
        printf("(%ld) ", tmp_num);
        // 022
        tmp = get_buf(table_id, tmp_num, 0);
        
        // case: leaf node
        if (tmp->frame.node.is_leaf) {
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                printf("%ld ", tmp->frame.node.key_values[i].key);
            }
            printf("| ");
        }
        else {
            enqueue(&q, tmp->frame.node.one_more_page_number);
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                printf("%ld ", tmp->frame.node.key_page_numbers[i].key);
                enqueue(&q, tmp->frame.node.key_page_numbers[i].page_number);
            }
            printf("| ");

            // case: last node in the level
            if (header_page->frame.header.root_page_number == tmp_num ||
                tmp->frame.node.key_page_numbers[0].key >= enter_key) {
                printf("\n");
                enter_key = tmp->frame.node.key_page_numbers[tmp->frame.node.number_of_keys - 1].key;
            }
        }
        // 022
        unpin(tmp);
    }
    printf("\n");
}