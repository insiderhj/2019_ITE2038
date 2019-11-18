#ifndef __LOCK_MANAGER_H__
#define __LOCK_MANAGER_H__

#include "bpt.hpp"

int begin_trx();
int end_trx(int tid);

#endif
