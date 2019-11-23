#ifndef __LOCK_MANAGER_HPP__
#define __LOCK_MANAGER_HPP__

#include "bpt.hpp"

int begin_trx();
int end_trx(int tid);

#endif
