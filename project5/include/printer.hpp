#ifndef __PRINTER_HPP__
#define __PRINTER_HPP__

#include "bpt.hpp"

// print
void enqueue(Queue* q, pagenum_t data);
pagenum_t dequeue(Queue* q);
int print_table(int table_id, char* pathname);

void print_buf();

#endif
