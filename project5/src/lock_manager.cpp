#include "bpt.hpp"

pthread_mutex_t trx_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_mutex = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::string, lock_t*> lock_table;
std::list<trx_t> trxs;
int max_tid = 1;

int begin_trx() {
    trx_t t;
    t.trx_id = __sync_fetch_and_add(&max_tid, 1);
    t.state = IDLE;
    // t.wait_lock = NULL;

    pthread_mutex_lock(&trx_mutex);
    trxs.push_back(t);
    pthread_mutex_unlock(&trx_mutex);
    return t.trx_id;
}

int end_trx(int tid) {
    pthread_mutex_lock(&trx_mutex);
    std::list<trx_t>::iterator it = std::find_if(trxs.begin(),
                                                 trxs.end(),
                                                 [tid](trx_t t){return t.trx_id == tid;});
    
    // not found
    if (it == trxs.end()) {
        pthread_mutex_unlock(&trx_mutex);
        return 0;
    }

    int sz = it->trx_locks.size();
    for (int i = 0; i < sz; i++) {
        release_lock(*it->trx_locks.begin());
    }
    trxs.erase(it);
    pthread_mutex_unlock(&trx_mutex);

    return tid;
}

trx_t* find_trx(int tid) {
    pthread_mutex_lock(&trx_mutex);
    std::list<trx_t>::iterator it = std::find_if(trxs.begin(),
                                                 trxs.end(),
                                                 [tid](trx_t t){return t.trx_id == tid;});
    
    if (it == trxs.end()) {
        pthread_mutex_unlock(&trx_mutex);
        return NULL;
    }

    trx_t* result = &(*it);
    pthread_mutex_unlock(&trx_mutex);
    return result;
}

lock_t* require_lock(int tid, pagenum_t pagenum, trx_t* trx, lock_mode mode) {
    lock_t* new_lock = new lock_t;
    new_lock->table_id = tid;
    new_lock->pagenum = pagenum;
    new_lock->mode = mode;
    new_lock->trx = trx;

    pthread_mutex_lock(&lock_mutex);

    trx->trx_locks.push_back(new_lock);
    lock_table.insert(std::make_pair(page_id(tid, pagenum), new_lock));
    
    return new_lock;
}

int release_lock(lock_t* lock) {
    std::list<lock_t*>* trx_locks = &(lock->trx->trx_locks);
    trx_locks->erase(std::find(trx_locks->begin(), trx_locks->end(), lock));
    lock_table.erase(page_id(lock->table_id, lock->pagenum));
    
    pthread_mutex_unlock(&lock_mutex);
    delete lock;

    return 0;
}
