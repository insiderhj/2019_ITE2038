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
 * 
 */
std::string page_id(int table_id, pagenum_t pagenum);

#endif
