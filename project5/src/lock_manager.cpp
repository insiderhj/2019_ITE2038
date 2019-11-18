#include "bpt.hpp"

std::list<trx_t> trxs;
int max_tid;

int begin_trx() {
    trx_t t;
    t.trx_id = __sync_fetch_and_add(&max_tid, 1);
    t.state = IDLE;
    t.wait_lock = NULL;

    trxs.push_back(t);
    return t.trx_id;
}

int end_trx(int tid) {
    std::list<trx_t>::iterator it = std::find_if(trxs.begin(),
                                                 trxs.end(),
                                                 [tid](trx_t t){return t.trx_id == tid;});
    
    // not found
    if (it == trxs.end()) return 0;

    // 여기 뭔가 써야되는데...
    
    trxs.erase(it);
    return tid;
}
