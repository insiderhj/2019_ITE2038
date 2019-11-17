#ifndef __PRINTER_H__
#define __PRINTER_H__

#include "bpt.hpp"

// print
void enqueue(Queue* q, pagenum_t data);
pagenum_t dequeue(Queue* q);
int print_table(int table_id);

void print_buf();

#endif
