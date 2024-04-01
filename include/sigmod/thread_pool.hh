#ifndef THREAD_POOL_HH
#define  THREAD_POOL_HH

#include<thread>
#include <functional>

struct ThreadPool {
    int64_t n_workers;
    std::vector<std::thread> pool;
};

ThreadPool MallocThreadPool(uint64_t n_workers, std::function<void ()> func);
void StartThreadPool(ThreadPool& threadPool);
void FreeThreadPool(ThreadPool& tp);

#endif

