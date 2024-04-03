#ifndef THREAD_POOL_HH
#define  THREAD_POOL_HH

#include<thread>
#include<functional>
#include<vector>

class ThreadPool{
    private:
        int64_t n_workers;
        std::vector<std::thread*> pool;
    public:
        ThreadPool(uint64_t n_workers);
        ~ThreadPool();
        void Start(std::function<void()> func);
};



#endif

