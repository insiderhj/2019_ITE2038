#ifndef __PRINTER_H__
#define __PRINTER_H__

#include "bpt.h"

// print
void enqueue(Queue* q, pagenum_t data);
pagenum_t dequeue(Queue* q);
void print_tree(int table_id);

#endif
