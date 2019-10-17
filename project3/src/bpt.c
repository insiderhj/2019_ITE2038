#include "bpt.h"

int fds[10];

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return the unique table id, which represents the own table in this database.
 * Otherwise, return negative value. (This table id will be used for future assignment.)
 */
int open_table(char* pathname) {
    // pathname is null
    if (pathname == NULL) return BAD_REQUEST;

    // file array is full
    if (fds[9]) return CONFLICT;

    int i = -1;
    while (fds[++i]);
    fds[i] = file_open(pathname);

    // open db failed
    if (fds[i] == -1) {
        fds[i] = 0;
        return INTERNAL_ERR;
    }

    page_t header_page;
    if (file_read_init(fds[i], &header_page) == 0) {
        memset(&header_page, 0, PAGE_SIZE);
        header_page.header.number_of_pages = 1;
        file_write_page(fds[i], 0, &header_page);
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
    page_t c;
    file_read_page(table_id, c_num, &c);

    // tree doesn't have root
    if (c_num == 0) return c_num;

    while (!c.node.is_leaf) {
        i = 0;
        while (i < c.node.number_of_keys) {
            if (key >= c.node.key_page_numbers[i].key) i++;
            else break;
        }
        if (i == 0) c_num = c.node.one_more_page_number;
        else {
            c_num = c.node.key_page_numbers[i - 1].page_number;
        }
        file_read_page(table_id, c_num, &c);
    }
    return c_num;
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’,
 * store matched ‘value’ string in ret_val and return 0.
 * Otherwise, return non-zero value.
 */
int db_find(int table_id, int64_t key, char* ret_val) {
    page_t header_page;
    file_read_page(table_id, 0, &header_page);
    int i = 0;
    pagenum_t leaf_num = find_leaf(table_id, header_page.header.root_page_number, key);
    page_t leaf;

    // leaf not found
    if (leaf_num == 0) return NOT_FOUND;

    file_read_page(table_id, leaf_num, &leaf);
    
    // search key in the leaf
    for (i = 0; i < leaf.node.number_of_keys; i++) {
        if (leaf.node.key_values[i].key == key) break;
    }
    
    // value not found
    if (i == leaf.node.number_of_keys) return NOT_FOUND;
    
    // copy value into ret_val
    strcpy(ret_val, leaf.node.key_values[i].value);
    return 0;
}

/* Creates a new node
 */
pagenum_t make_node(int table_id, page_t* node) {
    pagenum_t node_num = file_alloc_page(table_id);
    memset(node, 0, PAGE_SIZE);
    return node_num;
}

/* First insertion:
 * start a new tree.
 * returns new root's page number.
 */
pagenum_t start_new_tree(int table_id, int64_t key, char* value) {
    page_t root;
    pagenum_t root_num = make_node(table_id, &root);
    root.node.is_leaf = true;
    root.node.key_values[0].key = key;
    strcpy(root.node.key_values[0].value, value);
    root.node.number_of_keys++;

    file_write_page(table_id, root_num, &root);
    return root_num;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 * returns new root's page number
 */
pagenum_t insert_into_new_root(int table_id, pagenum_t left_num, page_t* left,
                               int64_t key, pagenum_t right_num, page_t* right) {
    page_t root;
    pagenum_t root_num = make_node(table_id, &root);
    root.node.key_page_numbers[0].key = key;
    root.node.one_more_page_number = left_num;
    root.node.key_page_numbers[0].page_number = right_num;
    root.node.number_of_keys++;

    // set parent numbers
    left->node.parent_page_number = root_num;
    right->node.parent_page_number = root_num;

    file_write_page(table_id, left_num, left);
    file_write_page(table_id, right_num, right);
    file_write_page(table_id, root_num, &root);

    return root_num;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to
 * the node to the left of the key to be inserted.
 */
int get_left_index(page_t* parent, pagenum_t left_num) {
    if (parent->node.one_more_page_number == left_num) return 0;

    int left_index = 1;
    while (left_index <= parent->node.number_of_keys
           && parent->node.key_page_numbers[left_index - 1].page_number != left_num)
        left_index++;
    
    return left_index;
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(int table_id, int parent_num, page_t* parent, int left_index, int64_t key, pagenum_t right_num) {
    int i;
    for (i = parent->node.number_of_keys; i > left_index; i--) {
        parent->node.key_page_numbers[i].page_number =
            parent->node.key_page_numbers[i - 1].page_number;
        parent->node.key_page_numbers[i].key = parent->node.key_page_numbers[i - 1].key;
    }
    parent->node.key_page_numbers[left_index].page_number = right_num;
    parent->node.key_page_numbers[left_index].key = key;
    parent->node.number_of_keys++;
    file_write_page(table_id, parent_num, parent);
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 * returns root page's page number
 */
pagenum_t insert_into_node_after_splitting(int table_id, pagenum_t root_num,
                                           pagenum_t parent_num, page_t* parent,
                                           int left_index, int64_t key, pagenum_t right_num) {
    int i, j, split;
    int64_t k_prime;
    page_t new_parent;
    pagenum_t new_parent_num;
    int64_t temp_keys[249];
    pagenum_t temp_page_numbers[250];

    // copy page numbers into temp arr
    temp_page_numbers[0] = parent->node.one_more_page_number;
    for (i = 0, j = left_index == 0 ? 2 : 1; i < parent->node.number_of_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp_page_numbers[j] = parent->node.key_page_numbers[i].page_number;
    }

    for (i = 0, j = 0; i < parent->node.number_of_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = parent->node.key_page_numbers[i].key;
    }

    // insert new key and page number
    temp_page_numbers[left_index + 1] = right_num;
    temp_keys[left_index] = key;

    split = cut(INTERNAL_ORDER);
    new_parent_num = make_node(table_id, &new_parent);
    
    // copy from temp arr to old parent
    parent->node.one_more_page_number = temp_page_numbers[0];
    for (i = 0; i < split - 1; i++) {
        parent->node.key_page_numbers[i].page_number = temp_page_numbers[i + 1];
        parent->node.key_page_numbers[i].key = temp_keys[i];
    }
    parent->node.number_of_keys = split - 1;
    k_prime = temp_keys[split - 1];

    // copy from temp arr to new parent
    new_parent.node.one_more_page_number = temp_page_numbers[++i];
    for (j = 0; i < INTERNAL_ORDER; i++, j++) {
        new_parent.node.key_page_numbers[j].page_number = temp_page_numbers[i + 1];
        new_parent.node.key_page_numbers[j].key = temp_keys[i];
    }
    new_parent.node.number_of_keys = INTERNAL_ORDER - split;
    new_parent.node.parent_page_number = parent->node.parent_page_number;

    // set parent node number of first child
    file_set_parent(table_id, new_parent.node.one_more_page_number, new_parent_num);

    // set parent node number of other children
    for (i = 0; i < new_parent.node.number_of_keys; i++) {
        file_set_parent(table_id, new_parent.node.key_page_numbers[i].page_number, new_parent_num);
    }

    file_write_page(table_id, parent_num, parent);
    file_write_page(table_id, new_parent_num, &new_parent);

    return insert_into_parent(table_id, root_num, parent_num, parent, k_prime, new_parent_num, &new_parent);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root page's page number after insertion.
 */
pagenum_t insert_into_parent(int table_id, pagenum_t root_num, pagenum_t left_num, page_t* left,
                             int64_t key, pagenum_t right_num, page_t* right) {
    int left_index;
    pagenum_t parent_num = left->node.parent_page_number;

    // case: left is root
    if (parent_num == 0) {
        return insert_into_new_root(table_id, left_num, left, key, right_num, right);
    }

    page_t parent;
    file_read_page(table_id, parent_num, &parent);
    left_index = get_left_index(&parent, left_num);

    if (parent.node.number_of_keys < INTERNAL_ORDER - 1) {
        insert_into_node(table_id, parent_num, &parent, left_index, key, right_num);
        return root_num;
    }

    return insert_into_node_after_splitting(table_id, root_num, parent_num, &parent,
                                            left_index, key, right_num);
}

/* Inserts a new pointer to a record and its corresponding key into a leaf.
 */
void insert_into_leaf(int table_id, pagenum_t leaf_num, page_t* leaf, int64_t key, char* value) {
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
    file_write_page(table_id, leaf_num, leaf);
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split in half.
 */
pagenum_t insert_into_leaf_after_splitting(int table_id, pagenum_t root_num, pagenum_t leaf_num, page_t* leaf,
                                      int64_t key, char* value) {
    page_t new_leaf;
    pagenum_t new_leaf_num;
    int temp_keys[LEAF_ORDER];
    char temp_values[LEAF_ORDER][VALUE_SIZE];
    int insertion_index, split, new_key, i, j;

    new_leaf_num = make_node(table_id, &new_leaf);
    new_leaf.node.is_leaf = true;

    insertion_index = 0;
    while (insertion_index < LEAF_ORDER - 1 && leaf->node.key_values[insertion_index].key < key) {
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

    new_leaf.node.right_sibling_page_number = leaf->node.right_sibling_page_number;
    leaf->node.right_sibling_page_number = new_leaf_num;

    new_leaf.node.parent_page_number = leaf->node.parent_page_number;
    new_key = new_leaf.node.key_values[0].key;

    file_write_page(table_id, leaf_num, leaf);
    file_write_page(table_id, new_leaf_num, &new_leaf);

    return insert_into_parent(table_id, root_num, leaf_num, leaf, new_key, new_leaf_num, &new_leaf);
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int db_insert(int table_id, int64_t key, char* value) {
    char res[VALUE_SIZE];
    
    if (table_id == 0) return BAD_REQUEST;
    if (db_find(table_id, key, res) == 0) return CONFLICT;

    page_t header_page;
    pagenum_t root_num;
    file_read_page(table_id, 0, &header_page);
    
    // case: file has no root page
    if (header_page.header.root_page_number == 0) {
        root_num = start_new_tree(table_id, key, value);
        file_set_root(table_id, root_num);
        return 0;
    }

    pagenum_t leaf_num = find_leaf(table_id, header_page.header.root_page_number, key);
    page_t leaf;
    file_read_page(table_id, leaf_num, &leaf);
    if (leaf.node.number_of_keys < LEAF_ORDER - 1) {
        insert_into_leaf(table_id, leaf_num, &leaf, key, value);
        return 0;
    }

    root_num = insert_into_leaf_after_splitting(
        table_id, header_page.header.root_page_number, leaf_num, &leaf, key, value);
    file_set_root(table_id, root_num);
    
    return 0;
}

void remove_entry_from_leaf(page_t* node, int64_t key) {
    int i;
    i = 0;

    while (node->node.key_values[i].key != key) i++;
    for (++i; i < node->node.number_of_keys; i++) {
        node->node.key_values[i - 1].key = node->node.key_values[i].key;
        strcpy(node->node.key_values[i - 1].value, node->node.key_values[i].value);
    }
    node->node.number_of_keys--;
}

void remove_entry_from_node(page_t* node, int64_t key) {
    int i;
    i = 0;

    while (node->node.key_page_numbers[i].key != key) i++;
    for (++i; i < node->node.number_of_keys; i++) {
        node->node.key_page_numbers[i - 1].key = node->node.key_page_numbers[i].key;
        node->node.key_page_numbers[i - 1].page_number =
            node->node.key_page_numbers[i].page_number;
    }
    node->node.number_of_keys--;
}

/* returns root page's page number
 */
pagenum_t adjust_root(int table_id, pagenum_t root_num) {
    pagenum_t new_root_num;
    page_t root, new_root;
    file_read_page(table_id, root_num, &root);

    // case: nonempty root
    if (root.node.number_of_keys > 0) return root_num;
    
    // case: empty root
    if (!root.node.is_leaf) {
        new_root_num = root.node.one_more_page_number;
        file_set_parent(table_id, new_root_num, 0);
    }
    else {
        new_root_num = 0;
    }
    file_free_page(table_id, root_num);

    return new_root_num;
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(page_t* parent, pagenum_t node_num) {
    int i;
    if (parent->node.one_more_page_number == node_num) return -1; 
    for (i = 0; i <= parent->node.number_of_keys; i++) {
        if (parent->node.key_page_numbers[i].page_number == node_num)
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
pagenum_t coalesce_nodes(int table_id, pagenum_t root_num, pagenum_t node_num, page_t* node,
                         pagenum_t neighbor_num, page_t* neighbor,
                         int neighbor_index, int64_t k_prime) {
    int i, j, neighbor_insertion_index, node_end;
    pagenum_t tmp_num;
    page_t* tmp;

    if (neighbor_index == -1) {
        tmp = node;
        node = neighbor;
        neighbor = tmp;

        tmp_num = node_num;
        node_num = neighbor_num;
        neighbor_num = tmp_num;
    }
    
    neighbor_insertion_index = neighbor->node.number_of_keys;

    // case: internal node
    if (!node->node.is_leaf) {
        neighbor->node.key_page_numbers[neighbor_insertion_index].key = k_prime;
        neighbor->node.number_of_keys++;

        node_end = node->node.number_of_keys;

        neighbor->node.key_page_numbers[neighbor_insertion_index].page_number =
            node->node.one_more_page_number;
        for (i = neighbor_insertion_index + 1, j = 0; j < node_end; i++, j++) {
            neighbor->node.key_page_numbers[i].key = node->node.key_page_numbers[j].key;
            neighbor->node.key_page_numbers[i].page_number =
                node->node.key_page_numbers[j].page_number;
        }
        neighbor->node.number_of_keys += node_end;

        for (i = neighbor_insertion_index; i < neighbor->node.number_of_keys; i++) {
            file_set_parent(table_id, neighbor->node.key_page_numbers[i].page_number, neighbor_num);
        }
    }

    // case: leaf node
    else {
        for (i = neighbor_insertion_index, j = 0; j < node->node.number_of_keys; i++, j++) {
            neighbor->node.key_values[i].key = node->node.key_values[j].key;
            strcpy(neighbor->node.key_values[i].value, node->node.key_values[j].value);
        }
        neighbor->node.number_of_keys += node->node.number_of_keys;
        neighbor->node.right_sibling_page_number = node->node.right_sibling_page_number;
    }

    file_write_page(table_id, neighbor_num, neighbor);
    root_num = delete_entry(table_id, root_num, node->node.parent_page_number, k_prime);
    file_free_page(table_id, node_num);
    return root_num;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the maximum
 * only called in internal node
 */
void redistribute_nodes(int table_id, pagenum_t node_num, page_t* node, pagenum_t neighbor_num, page_t* neighbor,
                        int neighbor_index, int k_prime_index, int64_t k_prime) {
    int i;
    page_t parent;
    file_read_page(table_id, node->node.parent_page_number, &parent);

    // case: node is the leftmost child
    // TODO
    if (neighbor_index == -1) {
        node->node.key_page_numbers[node->node.number_of_keys].key = k_prime;
        node->node.key_page_numbers[node->node.number_of_keys].page_number =
            neighbor->node.one_more_page_number;

        file_set_parent(table_id, node->node.key_page_numbers[node->node.number_of_keys].page_number, node_num);
        parent.node.key_page_numbers[k_prime_index].key =
            neighbor->node.key_page_numbers[0].key;
        
        neighbor->node.one_more_page_number = neighbor->node.key_page_numbers[0].page_number;
        for (i = 0; i < neighbor->node.number_of_keys - 1; i++) {
            neighbor->node.key_page_numbers[i].key =
                neighbor->node.key_page_numbers[i + 1].key;
            neighbor->node.key_page_numbers[i].page_number =
                neighbor->node.key_page_numbers[i + 1].page_number;
        }
    }

    // case: n has a neighbor to the left
    else {
        for (i = node->node.number_of_keys; i > 0; i--) {
            node->node.key_page_numbers[i].key = node->node.key_page_numbers[i - 1].key;
            node->node.key_page_numbers[i].page_number =
                node->node.key_page_numbers[i - 1].page_number;
        }
        node->node.key_page_numbers[0].page_number = node->node.one_more_page_number;

        node->node.one_more_page_number =
            neighbor->node.key_page_numbers[neighbor->node.number_of_keys - 1].page_number;
        
        file_set_parent(table_id, node->node.one_more_page_number, node_num);
        
        node->node.key_page_numbers[0].key = k_prime;
        parent.node.key_page_numbers[k_prime_index].key =
            neighbor->node.key_page_numbers[neighbor->node.number_of_keys - 1].key;
    }
    node->node.number_of_keys++;
    neighbor->node.number_of_keys--;

    file_write_page(table_id, node_num, node);
    file_write_page(table_id, neighbor_num, neighbor);
    file_write_page(table_id, node->node.parent_page_number, &parent);
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 * returns root page's page number after deletion.
 */
pagenum_t delete_entry(int table_id, pagenum_t root_num, pagenum_t node_num, int64_t key) {
    int min_keys;
    pagenum_t neighbor_num, parent_num;
    int neighbor_index;
    int k_prime_index;
    int64_t k_prime;
    int capacity;
    page_t node, neighbor, parent;
    file_read_page(table_id, node_num, &node);
    
    if (node.node.is_leaf) remove_entry_from_leaf(&node, key);
    else remove_entry_from_node(&node, key);
    file_write_page(table_id, node_num, &node);

    // case: node is root
    if (root_num == node_num)
        return adjust_root(table_id, root_num);

    // min_keys = node.node.is_leaf ? cut(LEAF_ORDER - 1) : cut(INTERNAL_ORDER) - 1;
    min_keys = 1;

    // case: number of keys is greater than or equal to min_keys
    if (node.node.number_of_keys >= min_keys)
        return root_num;

    // case: number of keys is lower than min_keys
    parent_num = node.node.parent_page_number;
    file_read_page(table_id, parent_num, &parent);
    
    neighbor_index = get_neighbor_index(&parent, node_num);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = parent.node.key_page_numbers[k_prime_index].key;
    switch (neighbor_index) {
        case -1:
            neighbor_num = parent.node.key_page_numbers[0].page_number;
            break;
        case 0:
            neighbor_num = parent.node.one_more_page_number;
            break;
        default:
            neighbor_num = parent.node.key_page_numbers[neighbor_index - 1].page_number;
            break;
    }
    file_read_page(table_id, neighbor_num, &neighbor);
    capacity = node.node.is_leaf ? LEAF_ORDER : INTERNAL_ORDER - 1;
    
    if (neighbor.node.number_of_keys + node.node.number_of_keys < capacity)
        return coalesce_nodes(table_id, root_num, node_num, &node, neighbor_num, &neighbor,
                              neighbor_index, k_prime);
    
    else {
        redistribute_nodes(table_id, node_num, &node, neighbor_num, &neighbor, neighbor_index,
                                  k_prime_index, k_prime);
        return root_num;
    }
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int db_delete(int table_id, int64_t key) {
    page_t header_page;
    file_read_page(table_id, 0, &header_page);
    pagenum_t leaf_num;
    char value[VALUE_SIZE];

    leaf_num = find_leaf(table_id, header_page.header.root_page_number, key);
    // no root or no such key
    if (leaf_num == 0 || db_find(table_id, key, value) == NOT_FOUND) return NOT_FOUND;

    header_page.header.root_page_number = delete_entry(table_id, header_page.header.root_page_number,
                                                        leaf_num, key);
    file_write_page(table_id, 0, &header_page);
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
    page_t header_page, tmp;
    int64_t enter_key;

    q.front = 0;
    q.rear = -1;
    q.item_count = 0;

    file_read_page(table_id, 0, &header_page);
    tmp_num = header_page.header.root_page_number;
    
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
        file_read_page(table_id, tmp_num, &tmp);
        
        // case: leaf node
        if (tmp.node.is_leaf) {
            for (i = 0; i < tmp.node.number_of_keys; i++) {
                printf("%ld ", tmp.node.key_values[i].key);
            }
            printf("| ");
        }
        else {
            enqueue(&q, tmp.node.one_more_page_number);
            for (i = 0; i < tmp.node.number_of_keys; i++) {
                printf("%ld ", tmp.node.key_page_numbers[i].key);
                enqueue(&q, tmp.node.key_page_numbers[i].page_number);
            }
            printf("| ");

            // case: last node in the level
            if (header_page.header.root_page_number == tmp_num ||
                tmp.node.key_page_numbers[0].key >= enter_key) {
                printf("\n");
                enter_key = tmp.node.key_page_numbers[tmp.node.number_of_keys - 1].key;
            }
        }
    }
    printf("\n");
}
