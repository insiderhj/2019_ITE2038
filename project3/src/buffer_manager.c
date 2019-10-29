#include "bpt.h"

buffer_pool_t buf_pool;
int init;

int init_db(int num_buf) {
    // pool already exists
    if (buf_pool.init) return CONFLICT;

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

void set_mru(int buf_num) {
    if (buf_pool.lru == buf_num) {
        buf_pool.lru = buf_pool.buffers[buf_num].next;
    }

    if (buf_pool.lru == -1) {
        buf_pool.lru = buf_num;
    }

    buf_pool.buffers[buf_num].next = -1;
    buf_pool.buffers[buf_num].prev = buf_pool.mru;
    
    if (buf_pool.mru != -1) {
        buf_pool.buffers[buf_pool.mru].next = buf_num;
    }
    buf_pool.mru = buf_num;
}

buffer_t* find_buf(int table_id, pagenum_t pagenum) {
    int i;
    buffer_t* buf = buf_pool.buffers + buf_pool.mru;
    while (buf->prev != -1) {
        if (buf->table_id == table_id && buf->page_num == pagenum) return buf;
        buf = buf_pool.buffers + buf->prev;
    }
    if (buf->table_id == table_id && buf->page_num == pagenum) return buf;
    return NULL;
}

buffer_t* get_buf(int table_id, pagenum_t pagenum) {
    if (!buf_pool.init) return NULL;

    int i;
    buffer_t* buf = find_buf(table_id, pagenum);
    buffer_t* tmp;

    // cannot find buf in pool
    if (!buf) {
        buf = add_buf();
    }

    file_read_page(table_id, pagenum, buf);
    set_mru(buf);
    return buf;
}

buffer_t* add_buf() {
    buffer_t* tmp;
    if (buf_pool.num_buffers == buf_pool.capacity) {
        flush_buf(buf_pool.lru);
        tmp = buf_pool.buffers + buf_pool.lru;
    } else {
        tmp = buf_pool.buffers + buf_pool.num_buffers;
    }
    tmp->is_dirty = 0;
    tmp->is_pinned = 0;

    return tmp;
}

buffer_t* buf_alloc_page(table_id) {
    buffer_t* header_page = get_buf(table_id, 0);
    header_page->is_dirty = 1;

    // case: no free page
    if (header_page->frame.header.free_page_number == 0) {
        // create new page
        buffer_t* new_page = add_buf();
        set_mru(new_page);
        new_page->table_id = table_id;
        new_page->page_num = header_page->frame.header.number_of_pages;

        // modify free page list
        new_page->frame.free.next_free_page_number = header_page->frame.header.free_page_number;
        header_page->frame.header.free_page_number = new_page->page_num;
        header_page->frame.header.number_of_pages++;
    }

    // get one free page from list
    buffer_t* buf = get_buf(table_id, header_page->frame.header.free_page_number);
    buf->is_dirty = 1;
    pagenum_t free_page_num = header_page->frame.header.free_page_number;
    file_read_page(table_id, free_page_num, buf);

    // modify free page list
    header_page->frame.header.free_page_number = buf->frame.free.next_free_page_number;
    return buf;
}

buffer_t* buf_free_page(int table_id, pagenum_t pagenum) {
    buffer_t* header_page, * p;
    header_page = get_buf(table_id, 0);
    p = get_buf(table_id, pagenum);

    header_page->is_dirty = 1;
    p->is_dirty = 1;
    memset(p, 0, PAGE_SIZE);
    
    pagenum_t first_free_page = header_page->frame.header.free_page_number;
    p->frame.free.next_free_page_number = first_free_page;
    header_page->frame.header.free_page_number = pagenum;
}

void flush_buf(buffer_t* buf) {
    if (buf->is_dirty) {
        file_write_page(buf->table_id, buf->page_num, &buf->frame);
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
    buffer_t* header_page = get_buf(table_id, 0);
    header_page->is_dirty = 1;
    header_page->frame.header.root_page_number = root_num;
}

int close_table(int table_id) {
    buffer_t* buf = buf_pool.buffers + buf_pool.lru;
    while (buf->next != -1) {
        
    }
}

int shutdown_db() {

}