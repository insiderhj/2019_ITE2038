#include "bpt.hpp"

int cut(int len) {
    return len % 2 == 0 ? len / 2 : len / 2 + 1;
}

std::string page_id(int table_id, pagenum_t pagenum) {
    return std::to_string(table_id) + "." + std::to_string(pagenum);
}
