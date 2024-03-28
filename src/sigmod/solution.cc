#include <sigmod/solution.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <iostream>
#include <map>
#include <algorithm>

void WriteSolution(Solution& solution, std::string output_path) {
    FILE* output = fopen(output_path.c_str(), "wb");

    Result* results_entry_point = solution.results;
    uint32_t results_to_write = solution.length;
    while(results_to_write > 0) {
        uint32_t this_batch = batch_size;
        if (this_batch > results_to_write) {
            this_batch = results_to_write;
        }
        fwrite(results_entry_point, sizeof(Result), this_batch, output);
        results_to_write -= this_batch;
        results_entry_point += this_batch;
    }

    fclose(output);
}

void FreeSolution(Solution& solution) {
    if (solution.results == nullptr)
        return;
    free(solution.results);
    solution.results = nullptr;
    solution.length = 0;
}
