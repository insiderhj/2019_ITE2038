#include "bpt.h"

int file_open(char* pathname) {
    return open(pathname, O_CREAT | O_NOFOLLOW | O_RDWR | O_SYNC, 0666);
}

int file_read_init(int table_id, buffer_t* header_page) {
    int res = read(table_id, header_page, PAGE_SIZE);
    header_page->table_id = table_id;
    header_page->page_num = 0;
    header_page->is_dirty = 0;
    header_page->is_pinned = 0;
    return res;
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

void file_set_parent(int table_id, pagenum_t pagenum, pagenum_t parent_num) {
    pwrite(table_id, &parent_num, 8, OFF(pagenum));
}
