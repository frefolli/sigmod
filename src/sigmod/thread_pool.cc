#include <sigmod/thread_pool.hh>

ThreadPool MallocThreadPool(
        uint64_t n_workers = std::thread::hardware_concurrency()) {

    ThreadPool* pool = (ThreadPool *) std::malloc(sizeof(ThreadPool));
    pool->n_workers = n_workers;

    return *pool;
}

void StartThreadPool(ThreadPool& tp,
        std::function<void()> func) {
    
    for (uint64_t i = 0; i < tp.n_workers; i++){
        tp.pool.push_back(std::thread(func));
    }
}

void FreeThreadPool(ThreadPool& tp){
    
    for (uint64_t i = 0; i < tp.n_workers; i++){
        tp.pool[i].join();
        delete &(tp.pool[i]);
    }

    delete &tp.pool;
    delete &tp;
}

