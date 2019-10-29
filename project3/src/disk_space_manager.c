#include "bpt.h"

int file_open(char* pathname) {
    return open(pathname, O_CREAT | O_NOFOLLOW | O_RDWR | O_SYNC, 0666);
}

// int file_read_init(int fd, page_t* header_page) {
//     return read(fd, header_page, PAGE_SIZE);
// }

/* Allocate an on-disk page from the free page list
 */
// TODO
pagenum_t file_alloc_page(int table_id) {
    page_t header_page;
    file_read_page(table_id, 0, &header_page);

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
        file_write_page(table_id, new_page_number, &new_page);
    }
    
    // get one free page from list
    page_t buf;
    int64_t free_page_number = header_page.header.free_page_number;
    file_read_page(table_id, free_page_number, &buf);

    // modify free page list
    header_page.header.free_page_number = buf.free.next_free_page_number;
    file_write_page(table_id, 0, &header_page);

    return free_page_number;
}

/* Free an on-disk page to the free page list
 */
void file_free_page(int table_id, pagenum_t pagenum) {
    page_t header_page;
    file_read_page(table_id, 0, &header_page);

    page_t p;
    memset(&p, 0, PAGE_SIZE);

    // make new free page, then modify free page list
    uint64_t first_free_page = header_page.header.free_page_number;
    p.free.next_free_page_number = first_free_page;
    header_page.header.free_page_number = pagenum;

    // write to disk
    file_write_page(table_id, 0, &header_page);
    file_write_page(table_id, pagenum, &p);
}

/* Read an on-disk page into the in-memory page structure(dest)
 */
void file_read_page(int table_id, pagenum_t pagenum, buffer_t* dest) {
    pread(table_id, dest, PAGE_SIZE, OFF(pagenum));
    dest->table_id = table_id;
    dest->page_num = pagenum;
    dest->is_dirty = 0;
    dest->is_pinned = 0;
}

/* Write an in-memory page(src) to the on-disk page
 */
void file_write_page(int table_id, pagenum_t pagenum, const buffer_t* src) {
    pwrite(table_id, src, PAGE_SIZE, OFF(pagenum));
}

void file_set_root(int table_id, pagenum_t root_num) {
    pwrite(table_id, &root_num, 8, 8);
}

void file_set_parent(int table_id, pagenum_t pagenum, pagenum_t parent_num) {
    pwrite(table_id, &parent_num, 8, OFF(pagenum));
}
