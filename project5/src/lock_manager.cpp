#include "bpt.hpp"

pthread_mutex_t mutex;
// std::unordered_map<pagenum_t, std::tuple<lock_t*, lock_t*> > lock_table;
std::list<trx_t> trxs;
int max_tid = 1;

int begin_trx() {
    trx_t t;
    t.trx_id = __sync_fetch_and_add(&max_tid, 1);
    t.state = IDLE;
    t.wait_lock = NULL;

    pthread_mutex_lock(&mutex);
    trxs.push_back(t);
    pthread_mutex_unlock(&mutex);
    return t.trx_id;
}
   
int end_trx(int tid) {
    std::list<trx_t>::iterator it = std::find_if(trxs.begin(),
                                                 trxs.end(),
                                                 [tid](trx_t t){return t.trx_id == tid;});
    
    // not found
    if (it == trxs.end()) return 0;


    pthread_mutex_lock(&mutex);
    trxs.erase(it);
    pthread_mutex_unlock(&mutex);

    return tid;
}
