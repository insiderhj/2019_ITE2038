#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include "bpt.hpp"

/**
 * Finds the appropriate place to split a node that is too big into two.
 * 
 * @param len original length to cut
 * @returns rounded value of len
 */
int cut(int len);

/**
 * Generate page id for given table id and page number.
 * 
 * @param table_id table_id to generate page id
 * @param pagenum page number to generate page id
 * @returns generated page id
 */
std::string page_id(int table_id, pagenum_t pagenum);

#endif
