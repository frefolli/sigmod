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
    
    std::map<uint32_t, std::pair<uint32_t, long long>> time_score_queries;
};

void FreeSolution(Solution& solution);
void WriteSolution(const Solution& solution, const std::string output_path);
Solution ReadSolution(const std::string input_path, const uint32_t legnth);
score_t CompareSolutions(const Solution& first, const Solution& second, const uint32_t n_queries);
score_t CompareSolutionsFromFiles(const std::string first, const std::string second, const uint32_t n_queries);
score_t CompareSolutions(const Database& database, const QuerySet& query_set, const Solution& expected, const Solution& got);

/* Assumed that output_path is a valid path*/
inline const std::string GenerateOutputPathFileName(const std::string output_path, const std::string prefix, const std::string suffix) {
    std::string out = output_path;
    if (!prefix.empty()) {
        std::size_t found = out.find_last_of('/');
        out.insert(found + 1, suffix);
    }
    if (!suffix.empty()) {
        std::size_t found = output_path.find_first_of('.');
        out.insert(found, suffix);
    }
    
    return out;
}

#endif
