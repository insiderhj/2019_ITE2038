#include "bpt.hpp"

buffer_pool_t buf_pool;
int init;

int init_db(int num_buf) {
    // size is equal to or less than zero
    if (num_buf <= 0) return BAD_REQUEST;

    if (init) {
        shutdown_db();
    }

    buf_pool.capacity = num_buf;
    buf_pool.mru = "";
    buf_pool.lru = "";

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

    std::string header_key = add_buf(table_id, 0);
    buffer_t* header = &(buf_pool.buffers.find(page_id(table_id, 0))->second);
    set_mru(header_key);
    if (!file_read_table(table_id, header)) {
        header->frame.header.number_of_pages = 1;
    }
    header->is_dirty = 1;
    unpin(header);
    return table_id;
}

void set_mru(std::string buf_key) {
    if (buf_pool.mru == buf_key) return;

    if (buf_pool.lru == "") {
        buf_pool.lru = buf_key;
    }
    else if (buf_pool.lru == buf_key) {
        buf_pool.lru = buf_pool.buffers[buf_key].next;
    }

    if (buf_pool.buffers[buf_key].next != "") {
        buf_pool.buffers[buf_pool.buffers[buf_key].next].prev = buf_pool.buffers[buf_key].prev;
    }
    
    if (buf_pool.buffers[buf_key].prev != "") {
        buf_pool.buffers[buf_pool.buffers[buf_key].prev].next = buf_pool.buffers[buf_key].next;
    }

    buf_pool.buffers[buf_key].next = "";
    if (buf_pool.mru != "") buf_pool.buffers[buf_pool.mru].next = buf_key;
    buf_pool.buffers[buf_key].prev = buf_pool.mru;
    buf_pool.mru = buf_key;
}

void unpin(buffer_t* buf) {
    buf->is_pinned = 0;
}

buffer_t* get_buf(int table_id, pagenum_t pagenum, uint32_t is_dirty) {
    if (!init) return NULL;

    buffer_t* buf;

    std::string buf_key = page_id(table_id, pagenum);
    auto it = buf_pool.buffers.find(buf_key);

    // found buf
    if (it != buf_pool.buffers.end()) {
        buf = &(it->second);
        buf->is_pinned = 1;
    }
    // cannot find
    else {
        buf_key = add_buf(table_id, pagenum);
        buf = &(buf_pool.buffers.find(buf_key)->second);
        file_read_page(table_id, pagenum, buf);
    }
    buf->is_dirty |= is_dirty;

    set_mru(buf_key);
    return buf;
}

std::string find_deletion_target() {
    std::string buf_key = buf_pool.lru;
    while (buf_pool.buffers[buf_key].is_pinned) {
        buf_key = buf_pool.buffers[buf_key].next;
        if (buf_key == "") exit(0);
    }
    return buf_key;
}

std::string add_buf(int table_id, pagenum_t pagenum) {
    buffer_t buf;
    std::string buf_key;

    // case: buffer pool is full
    if (buf_pool.buffers.size() == buf_pool.capacity) {
        buf_key = find_deletion_target();
        flush_buf(buf_key);
    }
    
    buf_key = page_id(table_id, pagenum);
    memset(&buf, 0, PAGE_SIZE);
    buf.is_pinned = 1;
    buf.is_dirty = 0;
    buf.next = "";
    buf.prev = "";

    buf_pool.buffers.insert(std::make_pair(buf_key, buf));
    return buf_key;
}

buffer_t* buf_alloc_page(buffer_t* header) {
    // case: no free page
    if (header->frame.header.free_page_number == 0) {
        // create new page
        std::string new_page_key = add_buf(header->table_id, header->frame.header.number_of_pages);
        buffer_t* new_page = &(buf_pool.buffers.find(new_page_key)->second);
        set_mru(new_page_key);
        new_page->table_id = header->table_id;
        new_page->page_num = header->frame.header.number_of_pages;

        // modify free page list
        new_page->frame.free.next_free_page_number = header->frame.header.free_page_number;
        header->frame.header.free_page_number = new_page->page_num;

        header->frame.header.number_of_pages++;
        unpin(new_page);
    }

    // get one free page from list
    buffer_t* buf = get_buf(header->table_id, header->frame.header.free_page_number, 1);

    // modify free page list
    header->frame.header.free_page_number = buf->frame.free.next_free_page_number;
    return buf;
}

void buf_free_page(buffer_t* header, buffer_t* p) {
    memset(p, 0, PAGE_SIZE);
    
    pagenum_t first_free_page = header->frame.header.free_page_number;
    p->frame.free.next_free_page_number = first_free_page;
    header->frame.header.free_page_number = p->page_num;
}

void flush_buf(std::string buf_key) {
    buffer_t* buf = &(buf_pool.buffers.find(buf_key)->second);
    buf->is_pinned = 1;
    if (buf->is_dirty) {
        file_write_page(buf->table_id, buf->page_num, buf);
    }

    // check if buf is mru
    if (buf_pool.mru == buf_key) {
        buf_pool.mru = buf->prev;
    }

    // check if buf is lru
    if (buf_pool.lru == buf_key) {
        buf_pool.lru = buf->next;
    }

    if (buf->next != "") {
        buf_pool.buffers[buf->next].prev = buf->prev;
    }

    if (buf->prev != "") {
        buf_pool.buffers[buf->prev].next = buf->next;
    }

    buf_pool.buffers.erase(buf_pool.buffers.find(buf_key));
}

void set_root(buffer_t* header, int root_num) {
    header->frame.header.root_page_number = root_num;
}

int close_table(int table_id) {
    if (!init) return BAD_REQUEST;
    if (check_fd(table_id) == NOT_FOUND) return NOT_FOUND;

    std::string buf_key = buf_pool.lru, next_key;
    if (buf_key == "") return 0;

    buffer_t* buf = &(buf_pool.buffers.find(buf_key)->second);
    while (buf->next != "") {
        next_key = buf->next;
        if (buf->table_id == table_id) flush_buf(buf_key);
        buf_key = next_key;
        buf = &(buf_pool.buffers.find(buf_key)->second);
    }
    if (buf->table_id == table_id) flush_buf(buf_key);

    int i = 0;
    for (i = 0; i < 10; i++) {
        if (fds[i] == table_id) fds[i] = 0;
    }
    return 0;
}

int shutdown_db() {
    if (!init) return BAD_REQUEST;

    std::string buf_key = buf_pool.lru, next_key;
    if (buf_key == "") return 0;

    buffer_t* buf = &(buf_pool.buffers.find(buf_key)->second);
    while (buf->next != "") {
        next_key = buf->next;
        flush_buf(buf_key);
        buf_key = next_key;
        buf = &(buf_pool.buffers.find(buf_key)->second);
    }
    flush_buf(buf_key);

    init = 0;
    return 0;
}
