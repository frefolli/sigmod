#include <sigmod/thread_pool.hh>

/*ThreadPool::ThreadPool() : ThreadPool::ThreadPool(std::thread::hardware_concurrency()) {
    
}*/

ThreadPool::ThreadPool(uint64_t n_workers) {
    n_workers = n_workers;
    //pool = *(new std::vector<std::thread>());

}

void ThreadPool::Start(std::function<void()> func) {
    for (uint64_t i = 0; i < n_workers; i++){
        pool.push_back(new std::thread(func));
    }
}

ThreadPool::~ThreadPool(){
    
    for (uint64_t i = 0; i < n_workers; i++){
        pool[i]->join();
        delete &pool[i];
    }

    delete &pool;
}

