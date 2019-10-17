#include "bpt.h"

int fd;
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