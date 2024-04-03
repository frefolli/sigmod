#include <sigmod/thread_pool.hh>

ThreadPool::ThreadPool(uint64_t n_workers) {
    n_workers = n_workers;
}

void ThreadPool::Start(std::function<void()> func) {
    for (uint64_t i = 0; i < n_workers; i++){
        pool.emplace_back(new std::thread(func));
    }
}

ThreadPool::~ThreadPool(){
    
    for (uint64_t i = 0; i < n_workers; i++){
        pool[i]->join();
        delete pool[i];
    }
    pool.clear();
}

