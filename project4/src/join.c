#include "bpt.h"

char pathnames[10][512];
int fds[10];

int open_join_file(char* pathname) {
    int i;
    for (i = 0; i < 10; i++) {
        if (fds[i] && strcmp(pathnames[i], pathname) == 0) return CONFLICT;
    }

    return open(pathname, O_CREAT | O_NOFOLLOW | O_RDWR | O_SYNC | O_TRUNC, 0666);
}

pagenum_t find_leftmost_leaf(int table_id, pagenum_t root_num) {
    int i;
    pagenum_t c_num = root_num;

    // tree doesn't have root
    if (c_num == 0) return c_num;

    buffer_t* c;
    c = get_buf(table_id, c_num, 0);

    while (!c->frame.node.is_leaf) {
        c_num = c->frame.node.one_more_page_number;
        unpin(c);
        c = get_buf(table_id, c_num, 0);
    }
    unpin(c);
    return c_num;
}

void advance(buffer_t** buf, int* idx) {
    buffer_t* tmp;
    (*idx)++;
    if (*idx == (*buf)->frame.node.number_of_keys) {
        tmp = get_buf((*buf)->table_id, (*buf)->frame.node.right_sibling_page_number, 0);
        unpin(*buf);
        *buf = tmp;
        *idx = 0;
    }
}

void write_csv(int fd, int64_t key_1, char* value_1, int64_t key_2, char* value_2) {
    dprintf(fd, "%d,%s,%d,%s\n", key_1, value_1, key_2, value_2);
}

int join_table(int table_id_1, int table_id_2, char * pathname) {
    int join_file = open_join_file(pathname);
    if (join_file == CONFLICT) return CONFLICT;
    if (join_file == -1) return INTERNAL_ERR;

    buffer_t* header_1, * header_2, * buf_1, * buf_2, * tmp;
    
    header_1 = get_buf(table_id_1, 0, 0);
    header_2 = get_buf(table_id_2, 0, 0);

    // get leftmost leaves' buffers
    buf_1 = get_buf(table_id_1, find_leftmost_leaf(table_id_1, header_1->frame.header.root_page_number), 0);
    buf_2 = get_buf(table_id_2, find_leftmost_leaf(table_id_2, header_2->frame.header.root_page_number), 0);
    unpin(header_1);
    unpin(header_2);

    int64_t key_1, key_2;
    int r, s;
    
    while (buf_1->page_num != 0 && buf_2->page_num != 0) {
        // if key[r] < key[s], advance r
        while (buf_1->page_num != 0
               && buf_1->frame.node.key_values[r].key < buf_2->frame.node.key_values[s].key) {
            advance(&buf_1, &r);
        }

        // if key[r] > key[s], advance s
        while (buf_2->page_num != 0
               && buf_1->frame.node.key_values[r].key > buf_2->frame.node.key_values[s].key) {
            advance(&buf_2, &s);
        }

        // if key[r] == key[s], write then advance r, s
        while (buf_1->page_num != 0 && buf_2->page_num != 0
               && buf_1->frame.node.key_values[r].key == buf_2->frame.node.key_values[s].key) {
            write_csv(join_file,
                      buf_1->frame.node.key_values[r].key,
                      buf_1->frame.node.key_values[r].value,
                      buf_2->frame.node.key_values[s].key,
                      buf_2->frame.node.key_values[s].value);
            advance(&buf_1, &r);
            advance(&buf_2, &s);
        }
    }

    unpin(buf_1);
    unpin(buf_2);
    return 0;
}
