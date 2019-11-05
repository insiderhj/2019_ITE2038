#include "bpt.h"

buffer_pool_t buf_pool;
char pathnames[10][512];
int fds[10];
int init;

int init_db(int num_buf) {
    // pool already exists
    if (init) return CONFLICT;

    // size is equal to or less than zero
    if (num_buf <= 0) return BAD_REQUEST;

    buf_pool.buffers = (buffer_t*)calloc(num_buf, sizeof(buffer_t));
    buf_pool.num_buffers = 0;
    buf_pool.capacity = num_buf;
    buf_pool.mru = -1;
    buf_pool.lru = -1;

    int i;
    for (i = 0; i < 10; i++) {
        fds[i] = 0;
    }

    init = 1;

    return 0;
}

int buf_read_table(char* pathname) {
    int table_id = file_open(pathname);
    if (table_id == -1) return table_id;

    int header_page_num = add_buf();
    buffer_t* header_page = buf_pool.buffers + header_page_num;
    set_mru(header_page_num);
    if (!file_read_init(table_id, header_page)) {
        memset(header_page, 0, PAGE_SIZE);
        header_page->frame.header.number_of_pages = 1;
    }
    header_page->is_dirty = 1;
    return table_id;
}

// TODO
void set_mru(int buf_num) {
    if (buf_pool.mru == buf_num) return;

    if (buf_pool.lru == -1) {
        buf_pool.lru = buf_num;
    }
    else if (buf_pool.lru == buf_num) {
        buf_pool.lru = buf_pool.buffers[buf_num].next;
    }

    if (buf_pool.buffers[buf_num].next != -1) {
        buf_pool.buffers[buf_pool.buffers[buf_num].next].prev = buf_pool.buffers[buf_num].prev;
    }
    
    if (buf_pool.buffers[buf_num].prev != -1) {
        buf_pool.buffers[buf_pool.buffers[buf_num].prev].next = buf_pool.buffers[buf_num].next;
    }

    buf_pool.buffers[buf_num].next = -1;
    if (buf_pool.mru != -1) buf_pool.buffers[buf_pool.mru].next = buf_num;
    buf_pool.buffers[buf_num].prev = buf_pool.mru;
    buf_pool.mru = buf_num;
}

void unpin(buffer_t* buf) {
    buf->is_pinned = 0;
}

int find_buf(int table_id, pagenum_t pagenum) {
    if (buf_pool.num_buffers == 0) return NOT_FOUND;

    int i, buf_num = buf_pool.mru;
    buffer_t* buf = buf_pool.buffers + buf_pool.mru;
    while (buf->prev != -1) {
        if (buf->table_id == table_id && buf->page_num == pagenum) return buf_num;
        buf_num = buf->prev;
        buf = buf_pool.buffers + buf_num;
    }
    if (buf->table_id == table_id && buf->page_num == pagenum) return buf_num;
    return NOT_FOUND;
}

buffer_t* get_buf(int table_id, pagenum_t pagenum, uint32_t is_dirty) {
    if (!init) return NULL;

    int i, buf_num = find_buf(table_id, pagenum);
    buffer_t* buf;

    // cannot find buf in pool
    if (buf_num == NOT_FOUND) {
        buf_num = add_buf();
        buf = buf_pool.buffers + buf_num;
        file_read_page(table_id, pagenum, buf);
    } else {
        buf = buf_pool.buffers + buf_num;
    }
    buf->is_pinned = 1;
    buf->is_dirty |= is_dirty;

    set_mru(buf_num);
    return buf;
}

int find_free_buf() {
    int i;
    for (i = 0; i < buf_pool.capacity; i++) {
        if (!buf_pool.buffers[i].is_allocated) return i;
    }
    return NOT_FOUND;
}

int find_deletion_target() {
    int buf_num = buf_pool.lru;
    while (buf_pool.buffers[buf_num].is_pinned) {
        buf_num = buf_pool.buffers[buf_num].next;
        if (buf_num < 0) exit(0);
    }
    return buf_num;
}

int add_buf() {
    buffer_t* buf;
    int buf_num;

    // case: buffer pool is full
    if (buf_pool.num_buffers == buf_pool.capacity) {
        buf_num = find_deletion_target();
        flush_buf(buf_num);
        buf = buf_pool.buffers + buf_num;
    }
    
    // case: buffer pool is not full
    else {
        buf_num = find_free_buf();
        buf = buf_pool.buffers + buf_pool.num_buffers;
    }
    buf = buf_pool.buffers + buf_num;
    buf->is_dirty = 0;
    buf->is_pinned = 0;
    buf->is_allocated = 1;
    buf->next = -1;
    buf->prev = -1;

    buf_pool.num_buffers++;

    return buf_num;
}

buffer_t* buf_alloc_page(buffer_t* header_page) {
    // case: no free page
    if (header_page->frame.header.free_page_number == 0) {
        // create new page
        int new_page_num = add_buf();
        buffer_t* new_page = buf_pool.buffers + new_page_num;
        set_mru(new_page_num);
        new_page->table_id = header_page->table_id;
        new_page->page_num = header_page->frame.header.number_of_pages;

        // modify free page list
        new_page->frame.free.next_free_page_number = header_page->frame.header.free_page_number;
        header_page->frame.header.free_page_number = new_page->page_num;

        header_page->frame.header.number_of_pages++;
    }

    // get one free page from list
    buffer_t* buf = get_buf(header_page->table_id, header_page->frame.header.free_page_number, 1);
    pagenum_t free_page_num = header_page->frame.header.free_page_number;
    // file_read_page(table_id, free_page_num, buf);

    // modify free page list
    header_page->frame.header.free_page_number = buf->frame.free.next_free_page_number;
    return buf;
}

void buf_free_page(buffer_t* header_page, buffer_t* p) {
    memset(p, 0, PAGE_SIZE);
    
    pagenum_t first_free_page = header_page->frame.header.free_page_number;
    p->frame.free.next_free_page_number = first_free_page;
    header_page->frame.header.free_page_number = p->page_num;
}

void flush_buf(int buf_num) {
    buffer_t* buf = buf_pool.buffers + buf_num;
    buf->is_pinned = 1;
    if (buf->is_dirty) {
        file_write_page(buf->table_id, buf->page_num, buf);
    }

    // check if buf is mru
    if (buf_pool.mru == buf_num) {
        buf_pool.mru = buf->prev;
    }

    // check if buf is lru
    if (buf_pool.lru == buf_num) {
        buf_pool.lru = buf->next;
    }

    if (buf->next != -1) {
        buf_pool.buffers[buf->next].prev = buf->prev;
    }

    if (buf->prev != -1) {
        buf_pool.buffers[buf->prev].next = buf->next;
    }

    buf->is_allocated = 0;
    buf_pool.num_buffers--;
}

void set_root(buffer_t* header_page, int root_num) {
    header_page->frame.header.root_page_number = root_num;
}

int close_table(int table_id) {
    int buf_num = buf_pool.lru, next_num;
    buffer_t* buf = buf_pool.buffers + buf_num;
    while (buf->next != -1) {
        next_num = buf->next;
        if (buf->table_id == table_id) flush_buf(buf_num);
        buf_num = next_num;
        buf = buf_pool.buffers + buf_num;
    }
    if (buf->table_id == table_id) flush_buf(buf_num);

    int i = 0;
    for (i = 0; i < 10; i++) {
        if (fds[i] == table_id) fds[i] = 0;
    }
    return 0;
}

int shutdown_db() {
    int buf_num = buf_pool.lru, next_num;
    buffer_t* buf = buf_pool.buffers + buf_num;
    while (buf->next != -1) {
        next_num = buf->next;
        flush_buf(buf_num);
        buf_num = next_num;
        buf = buf_pool.buffers + buf_num;
    }
    flush_buf(buf_num);

    free(buf_pool.buffers);

    init = 0;
    return 0;
}

void print_buf() {
    printf("buf info - mru: %d, lru: %d\n", buf_pool.mru, buf_pool.lru);
    int buf_num = buf_pool.mru;
    buffer_t* buf = buf_pool.buffers + buf_num;
    while (buf->prev != -1) {
        printf("buf_num: %d, table_id: %d, pagenum: %d\n", buf_num, buf->table_id, buf->page_num);
        buf_num = buf->prev;
        buf = buf_pool.buffers + buf_num;
    }
    printf("buf_num: %d, table_id: %d, pagenum: %d\n", buf_num, buf->table_id, buf->page_num);
}
