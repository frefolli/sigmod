#ifndef THREAD_POOL_HH
#define THREAD_POOL_HH
#include <vector>
#include <map>
#include <iostream>
#include <cstdint>
#include <mutex>
#include <thread>
#include <functional>
#include <deque>
#include <sigmod/c_map.hh>
#include <sigmod/tweaks.hh>

struct ThreadPool {
    uint32_t n_of_workers = 1;
    std::map<uint32_t, std::thread*> threads = {};
    std::deque<uint32_t> finished = {};
    std::mutex mutex;

    uint32_t id_counter = 0;

    ThreadPool(uint32_t n_of_workers = 5);

    void release(uint32_t ID);
    void erase();

    void run(std::function<void(typename c_map_t::const_iterator)> fun, const c_map_t& iterable);
    void run(std::function<void(uint32_t)> fun, uint32_t start, uint32_t end);
};

/*
void foo_one() {
    typedef std::map<uint32_t, std::pair<uint32_t, uint32_t>> c_map_t;
    c_map_t C_map;
    for (uint32_t i = 0; i < 40; i++) {
        C_map[i] = {i * 200, i * 200 + 200};
    }
    ThreadPool pool (2);
    pool.run([](typename c_map_t::iterator it) {
            std::cout << it->first << std::endl;
    }, C_map);
}

void foo_two() {
    uint32_t length = 50;
    float* buffer = (float*) malloc (sizeof(float) * length);
    ThreadPool pool (2);
    pool.run([&buffer](uint32_t i) {
        buffer[i] = 17.07;
    }, (uint32_t) 0, length);
    for (uint32_t i = 0; i < length; i++) {
        std::cout << buffer[i] << std::endl;
    }
    free(buffer);
}
*/

#endif
