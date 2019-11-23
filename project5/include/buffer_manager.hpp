#ifndef __BUFFER_MANAGER_HPP__
#define __BUFFER_MANAGER_HPP__

#include "bpt.hpp"

/**
 * Allocate the buffer pool (array) with the given number of entries and initialize other fields.
 * 
 * @param num_buf size of buffer pool
 * @returns 0 if success. Otherwise, return negative value.
 */
int init_db(int num_buf);

/**
 * Read new table with pathname.
 * 
 * @param pathname pathname of new table
 * @returns fd of opened table
 */
int buf_read_table(char* pathname);

/**
 * Set buffer with buf_num as most recently used buffer.
 * 
 * @param buf_key buffer's key in buffer pool
 */
void set_mru(std::string buf_key);

/**
 * unpin buffer
 * 
 * @param buf pinned buffer
 */
void unpin(buffer_t* buf);

/**
 * Get buffer matching given table_id and page number.
 * set dirty bit if is_dirty is 1
 * 
 * @param table_id searching table id
 * @param pagenum searching page number
 * @returns matched buffer if success. Otherwise, return NULL
 */
buffer_t* get_buf(int table_id, pagenum_t pagenum, uint32_t is_dirty);

/**
 * Find least recently used buffer which is not pinned
 * 
 * @returns key of deletion target buffer
 */
std::string find_deletion_target();

/**
 * Add new buffer space in buffer pool.
 * If there is no free buffer, delete one and add in the space.
 * 
 * @param table_id table id for new buffer
 * @param pagenum pagenum for new buffer
 * @returns index of new buffer in buffer pool
 */
std::string add_buf(int table_id, pagenum_t pagenum);

/**
 * Alloc new free page in table.
 * 
 * @param header_page
 * @returns allocated buffer if success. Otherwise, return NULL
 */
buffer_t* buf_alloc_page(buffer_t* header);

/**
 * Free a page and insert the page in free page list
 * 
 * @param header_page pinned header page of current table
 * @param p buffer to free
 */
void buf_free_page(buffer_t* header, buffer_t* p);

/**
 * Free a buffer. if dirty bit is set, write on disk
 * 
 * @param buf_key
 */
void flush_buf(std::string buf_key);

/**
 * Change root number of table
 * 
 * @param header_page pinned header page of current table
 * @param root_num new root number
 */
void set_root(buffer_t* header_page, int root_num);

/**
 * Flush all buffers with given table id.
 * 
 * @param table_id table id to delete
 * @returns 0 if success. Otherwise, return negative value
 */
int close_table(int table_id);

/**
 * Flush all buffers in buffer pool, then reset init.
 * 
 * @returns 0 if success. Otherwise, return negative value
 */
int shutdown_db();

#endif
