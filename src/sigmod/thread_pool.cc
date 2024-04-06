#include <sigmod/thread_pool.hh>

ThreadPool::ThreadPool(uint32_t n_of_workers) : n_of_workers(n_of_workers) {}

void ThreadPool::release(uint32_t ID) {
    std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
    finished.push_front(ID);
    delete guard;
}

void ThreadPool::erase() {
    uint32_t ID = finished.back();
    finished.pop_back();
    threads[ID]->join();
    delete threads[ID];
    threads.erase(ID);
}

void ThreadPool::run(std::function<void(uint32_t)> fun, uint32_t start, uint32_t end) {
    uint32_t length = end - start;
    uint32_t shift = length / n_of_workers;
    uint32_t start_i = 0;
    uint32_t end_i = shift;
    while (start_i < end) {
        if (threads.size() >= n_of_workers) {
            while(finished.size() == 0);
            while(finished.size() > 0) {
                erase();
            }
        }
        uint32_t ID = id_counter++;
        threads[ID] = new std::thread([&fun, this, ID, start_i, end_i]() {
            for (uint32_t i = start_i; i < end_i; i++) {
                fun(i);
            }
            release(ID);
        });
        start_i = end_i;
        end_i = start_i + shift;
        if (end_i > end)
            end_i = end;
    }
    while(threads.size() > 0) {
        while(finished.size() > 0) {
            erase();
        }
    }
}

void ThreadPool::run(std::function<void(typename c_map_t::const_iterator)> fun, const c_map_t& iterable) {
    for (auto it = iterable.begin(); it != iterable.end(); it++) {
        while (threads.size() >= n_of_workers) {
            std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
            while(finished.size() > 0) {
                erase();
            }
            delete guard;
        }
        uint32_t ID = id_counter++;
        threads[ID] = new std::thread([&fun, this, ID, it]() {
            fun(it);
            release(ID);
        });
    }
    while(threads.size() > 0) {
        std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
        while(finished.size() > 0) {
            erase();
        }
        delete guard;
   }
}
