#include "bpt.h"

int fd = 0;
header_page_t header_page;

void set_next_free_page_number(page_t* free_page, uint64_t next_free_page) {
    ((free_page_t*)free_page)->next_free_page_number = next_free_page;
}

pagenum_t file_alloc_page() {
    file_read_page(0, &header_page);

    // free page가 없을때 새로 만들어
    if (header_page.free_page_number == 0) {
        free_page_t new_page;
        int64_t new_page_number = header_page.number_of_pages;
        memset(&new_page, 0, PAGE_SIZE);

        new_page.next_free_page_number = header_page.free_page_number;
        header_page.free_page_number = new_page_number;
        header_page.number_of_pages++;
        file_write_page(0, &header_page);
        file_write_page(new_page_number, &new_page);
    }
    
    free_page_t buf;
    int64_t free_page_number = header_page.free_page_number;
    file_read_page(free_page_number, &buf);
    header_page.free_page_number = buf.next_free_page_number;
    file_write_page(0, &header_page);

    return free_page_number;
}

void file_free_page(pagenum_t pagenum) {
    file_read_page(0, &header_page);

    page_t tmp; 
    memset(&tmp, 0, PAGE_SIZE);

    uint64_t first_free_page = header_page.free_page_number;
    set_next_free_page_number(&tmp, first_free_page);
    header_page.free_page_number = pagenum;

    file_write_page(0, &header_page);
    file_write_page(pagenum, &tmp);
}

void file_read_page(pagenum_t pagenum, page_t* dest) {
    pread(fd, dest, PAGE_SIZE, OFF(pagenum));
}

void file_write_page(pagenum_t pagenum, const page_t* src) {
    pwrite(fd, src, PAGE_SIZE, OFF(pagenum));
}

int open_table(char* pathname) {
    // pathname is null
    if (!pathname) return -1;

    // fd already opened
    if (fd) return -2;

    int fd = open(pathname, O_CREAT | O_NOFOLLOW | O_RDWR | O_SYNC);

    // open db failed
    if (fd == -1) return -3;

    if (read(fd, &header_page, PAGE_SIZE) == 0) {
        memset(&header_page, 0, PAGE_SIZE);
        header_page.number_of_pages = 1;
        file_write_page(0, &header_page);
    }

    return 0;
}

int db_insert(int64_t key, char* value) {

}

int db_find(int64_t key, char* ret_val) {

}

int db_delete(int64_t key) {

}
