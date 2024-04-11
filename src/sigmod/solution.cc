#include <sigmod/solution.hh>
#include <sigmod/flags.hh>
#include <sigmod/stats.hh>
#include <sigmod/scoreboard.hh>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <sigmod/tweaks.hh>

void WriteSolution(const Solution& solution, const std::string output_path) {
    FILE* output = fopen(output_path.c_str(), "wb");

    Result* results_entry_point = solution.results;
    uint32_t results_to_write = solution.length;
    while(results_to_write > 0) {
        uint32_t this_batch = BATCH_SIZE;
        if (this_batch > results_to_write) {
            this_batch = results_to_write;
        }
        fwrite(results_entry_point, sizeof(Result), this_batch, output);
        results_to_write -= this_batch;
        results_entry_point += this_batch;
    }

    fclose(output);
}

Solution ReadSolution(const std::string input_path, const uint32_t length) {
    FILE* input = fopen(input_path.c_str(), "rb");
    Solution solution = {
        .length = length,
        .results = (Result*) malloc(sizeof(Result) * length)
    };

    Result* results_entry_point = solution.results;
    uint32_t results_to_read = solution.length;
    while(results_to_read > 0) {
        uint32_t this_batch = BATCH_SIZE;
        if (this_batch > results_to_read) {
            this_batch = results_to_read;
        }
        fread(results_entry_point, sizeof(Result), this_batch, input);
        results_to_read -= this_batch;
        results_entry_point += this_batch;
    }
    fclose(input);
    return solution;
}

void FreeSolution(Solution& solution) {
    if (solution.results == nullptr)
        return;
    free(solution.results);
    solution.results = nullptr;
    solution.length = 0;
}

score_t CompareSolutions(const Solution& expected, const Solution& got, const uint32_t n_queries) {
    uint32_t length = std::min(expected.length, got.length);
    score_t recall = 0;
    for (uint32_t i = 0; i < length; i++) {
        
        if (i >= n_queries)
            break;

        for (uint32_t j = 0; j < k_nearest_neighbors; j++) {
            if (expected.results[i].data[j] == got.results[i].data[j]) {
                recall++;
            }
        }
    }

    recall /= (std::min(n_queries, length) * k_nearest_neighbors);

    return recall;
}

score_t CompareSolutionsFromFiles(const std::string expected, const std::string got, const uint32_t n_queries) {
    Solution expected_solutions = ReadSolution(expected, n_queries);
    Solution got_solutions = ReadSolution(got, n_queries);

    score_t recall = CompareSolutions(expected_solutions, got_solutions, n_queries);

    FreeSolution(expected_solutions);
    FreeSolution(got_solutions);
    return recall;
}

score_t CompareSolutions(const Database& database, const QuerySet& query_set, const Solution& expected, const Solution& got) {
    uint32_t length = std::min(expected.length, got.length);
    score_t recall = 0;
    for (uint32_t i = 0; i < length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        std::cout << "Q := " << query_set.queries[i].query_type
                  << " | " << query_set.queries[i].v
                  << " | " << query_set.queries[i].l
                  << " | " << query_set.queries[i].r
                  << std::endl;
        for (uint32_t j = 0; j < k_nearest_neighbors; j++) {
            if (expected.results[i].data[j] == got.results[i].data[j]) {
                recall++;
            } else {
                #ifdef SHOW_MISMATCH_IN_COMPARISON
                const uint32_t a = expected.results[i].data[j];
                const uint32_t b = got.results[i].data[j];
                const Query& query = query_set.queries[i];
                std::cout << "Solution conflit! with i = " << i << ", j = " << j << "; "
                          
                          << distance(query, database.records[a])
                          << " vs "
                          << distance(query, database.records[b])
                          << "; "

                          << a << " vs "
                          << b << "; "

                          << database.records[a].T << " vs "
                          << database.records[b].T << "; "
                          << std::endl;
                #endif
            }
        }
    }

    #ifdef STOP_AFTER_TOT_ELEMENTS
        recall /= (std::min((uint32_t) TOT_ELEMENTS, length) * k_nearest_neighbors);
    #else
        recall /= (length * k_nearest_neighbors);
    #endif

    return recall;
}
