#ifndef SOLUTION_HH
#define SOLUTION_HH

#include <sigmod/config.hh>
#include <string>

struct Result {
    uint32_t data[k_nearest_neighbors];
};

struct Solution {
    uint32_t length;
    Result* results;
};

void FreeSolution(Solution& solution);
void WriteSolution(Solution& solution, std::string output_path);

#endif
