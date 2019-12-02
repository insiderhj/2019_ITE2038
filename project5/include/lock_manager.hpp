#ifndef __LOCK_MANAGER_HPP__
#define __LOCK_MANAGER_HPP__

#include "bpt.hpp"

int begin_trx();
int end_trx(int tid);
trx_t* find_trx(int tid);

lock_t* require_lock(int tid, pagenum_t pagenum, trx_t* trx, lock_mode mode);
int release_lock(lock_t* lock);

#endif
