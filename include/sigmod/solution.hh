#ifndef SOLUTION_HH
#define SOLUTION_HH

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query_set.hh>
#include <string>

struct Result {
    uint32_t data[k_nearest_neighbors];
};

struct Solution {
    uint32_t length;
    Result* results;
};

void FreeSolution(Solution& solution);
void WriteSolution(const Solution& solution, const std::string output_path);
score_t CompareSolutions(const Database& database, const QuerySet& query_set, const Solution& expected, const Solution& got);

#endif
