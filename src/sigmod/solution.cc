#include <sigmod/solution.hh>
#include <sigmod/stats.hh>
#include <sigmod/scoreboard.hh>
#include <cstdio>
#include <iostream>
#include <map>
#include <algorithm>
#include <cmath>

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

void CompareSolutions(const Database& database, const QuerySet& query_set, const Solution& expected, const Solution& got) {
    uint32_t length = expected.length;
    for (uint32_t i = 0; i < length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        for (uint32_t j = 0; j < k_nearest_neighbors; j++) {
            if (expected.results[i].data[j] != got.results[i].data[j]) {
                std::cout << "Solution conflit! with i = " << i << ", j = " << j << "; "
                          << got.results[i].data[j] << " io " << expected.results[i].data[j] << "; "
                          << distance(query_set.queries[i], database.records[got.results[i].data[j]])
                          << " vs "
                          << distance(query_set.queries[i], database.records[expected.results[i].data[j]])
                          << std::endl;
                break;
            }
        }
    }
}
