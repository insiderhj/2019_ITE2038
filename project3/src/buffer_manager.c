#include "bpt.h"

buffer_pool_t buf_pool;
int init;

//TODO: dirty bit sets

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
        file_read_page(table_id, pagenum, buf);
    }

    buf = buf_pool.buffers + buf_num;
    
    buf->is_dirty |= is_dirty;

    set_mru(buf_num);
    return buf;
}

int add_buf() {
    buffer_t* buf;
    int buf_num;

    // case: buffer pool is full
    if (buf_pool.num_buffers == buf_pool.capacity) {
        flush_buf(buf_pool.buffers + buf_pool.lru);
        buf_num = buf_pool.lru;
        buf = buf_pool.buffers + buf_pool.lru;
    }
    
    // case: buffer pool is not full
    else {
        buf_num = buf_pool.num_buffers;
        buf = buf_pool.buffers + buf_pool.num_buffers;
        buf_pool.num_buffers++;
    }
    buf = buf_pool.buffers + buf_num;
    buf->is_dirty = 0;
    buf->is_pinned = 0;
    buf->next = -1;
    buf->prev = -1;

    return buf_num;
}

buffer_t* buf_alloc_page(table_id) {
    buffer_t* header_page = get_buf(table_id, 0, 1);

    // case: no free page
    if (header_page->frame.header.free_page_number == 0) {
        // create new page
        int new_page_num = add_buf();
        buffer_t* new_page = buf_pool.buffers + new_page_num;
        set_mru(new_page_num);
        new_page->table_id = table_id;
        new_page->page_num = header_page->frame.header.number_of_pages;

        // modify free page list
        new_page->frame.free.next_free_page_number = header_page->frame.header.free_page_number;
        header_page->frame.header.free_page_number = new_page->page_num;
        header_page->frame.header.number_of_pages++;
    }

    // get one free page from list
    buffer_t* buf = get_buf(table_id, header_page->frame.header.free_page_number, 1);
    pagenum_t free_page_num = header_page->frame.header.free_page_number;
    file_read_page(table_id, free_page_num, buf);

    // modify free page list
    header_page->frame.header.free_page_number = buf->frame.free.next_free_page_number;
    return buf;
}

void buf_free_page(int table_id, pagenum_t pagenum) {
    buffer_t* header_page, * p;
    header_page = get_buf(table_id, 0, 1);
    p = get_buf(table_id, pagenum, 1);

    memset(p, 0, PAGE_SIZE);
    
    pagenum_t first_free_page = header_page->frame.header.free_page_number;
    p->frame.free.next_free_page_number = first_free_page;
    header_page->frame.header.free_page_number = pagenum;
}

void flush_buf(buffer_t* buf) {
    if (buf->is_dirty) {
        file_write_page(buf->table_id, buf->page_num, buf);
    }

    // check if buf is mru
    if (buf->next == -1) {
        buf_pool.mru = buf->prev;
    } else {
        buf_pool.buffers[buf->next].prev = buf->prev;
    }

    // check if buf is lru
    if (buf->prev == -1) {
        buf_pool.lru = buf->next;
    } else {
        buf_pool.buffers[buf->prev].next = buf->next;
    }
}

void set_root(int table_id, int root_num) {
    buffer_t* header_page = get_buf(table_id, 0, 1);
    header_page->frame.header.root_page_number = root_num;
}

int close_table(int table_id) {
    buffer_t* buf = buf_pool.buffers + buf_pool.lru, * next;
    while (buf->next != -1) {
        next = buf_pool.buffers + buf->next;
        if (buf->table_id == table_id) flush_buf(buf);
        buf = next;
    }
    if (buf->table_id == table_id) flush_buf(buf);
    return 0;
}

int shutdown_db() {
    buffer_t* buf = buf_pool.buffers + buf_pool.lru, * next;
    while (buf->next != -1) {
        next = buf_pool.buffers + buf->next;
        flush_buf(buf);
        buf = next;
    }
    flush_buf(buf);
    free(buf_pool.buffers);
    init = 0;
    return 0;
}