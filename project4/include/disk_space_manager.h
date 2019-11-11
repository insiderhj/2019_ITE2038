#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "bpt.h"

/**
 * Check if given pathname is valid
 * 
 * @param pathname checking pathname
 * @returns 0 if success. Otherwise, return negative value
 */
int check_pathname(char* pathname);

/**
 * Check if given fd is in array
 * 
 * @param fd checking fd
 * @returns 0 if given fd is in array. Otherwise, return negative value
 */
int check_fd(int fd);

/**
 * Open file with given pathname
 * 
 * @param pathname pathname of opening file
 * @returns fd of opened file
 */
int file_open(char* pathname);

/**
 * Read header page of table from disk
 * 
 * @param table_id unique table id of reading file
 * @param header_page save header page data in here
 * @returns result of read function
 */
int file_read_table(int table_id, buffer_t* header_page);

/**
 * Read single page from disk
 * 
 * @param table_id unique table id of reading page
 * @param pagenum reading page's page number
 * @param dest pinned buffer to save result in
 */
void file_read_page(int table_id, pagenum_t pagenum, buffer_t* dest);

/**
 * Write single page to disk
 * 
 * @param table_id unique table id of writing page
 * @param pagenum writing page's page number
 * @param src pinned buffer to write to disk
 */
void file_write_page(int table_id, pagenum_t pagenum, const buffer_t* src);

#endif
