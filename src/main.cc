#include <sigmod.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <queue>
#include <vector>
#include <map>

inline score_t distance(const Query& query, const Record& record) {
    score_t sum = 0;
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        sum += pow(query.fields[i] - record.fields[i], 2);
    }
    return sum;
}

typedef struct {
    uint32_t index;
    score_t score;
} Candidate;

const auto compare_function = [](Candidate a, Candidate b) { return a.score < b.score; };
typedef std::priority_queue<Candidate, std::vector<Candidate>, decltype(compare_function)> Scoreboard;

void OutputSolution(FILE* solution, Scoreboard& scoreboard) {
    static uint32_t* buffer = nullptr;

    if (buffer == nullptr)
        buffer = (uint32_t*) malloc(sizeof(uint32_t) * k_nearest_neighbors);

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        buffer[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
    fwrite(solution, sizeof(uint32_t), k_nearest_neighbors, solution);
}

inline void PushCandidate(Scoreboard& scoreboard, Database& database, Query& query, uint32_t record_index) {
    scoreboard.emplace(Candidate({
        .index = record_index,
        .score = distance(query, database.records[record_index])
    }));
}

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

enum query_t {
    NORMAL = 0,
    BY_C = 1,
    BY_T = 2,
    BY_C_AND_T = 3
};

void FindForQuery(FILE* solution, Database& database, std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map, Query& query) {
    const uint32_t query_type = (uint32_t) (query.query_type);

    uint32_t start_index = 0;
    uint32_t end_index = database.length;

    if (query_type == BY_C || query_type == BY_C_AND_T) {
        start_index = C_map[query.v].first;
        end_index = C_map[query.v].second + 1;
    }

    // maximum distance in the front
    Scoreboard scoreboard(compare_function);

    for (uint32_t i = start_index; i < end_index; i++) {
        if (query_type == NORMAL || query_type == BY_C ||
            elegible_by_T(query, database.records[i])) {

            PushCandidate(scoreboard, database, query, i);
            if (scoreboard.size() > k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    }

    OutputSolution(solution, scoreboard);
}

void SolveForQueries(std::string output_path, Database& database, std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map, QuerySet& query_set) {
    FILE* solution = fopen(output_path.c_str(), "wb");
    for (uint32_t i = 0; i < query_set.length; i++) {
        FindForQuery(solution, database, C_map, query_set.queries[i]);
        if (i % 1000 == 0)
            std::cout << "progress = " << i << " of " << query_set.length << std::endl;
    }
}

int main(int argc, char** args) {
    std::string database_path = "dummy-data.bin";
    std::string query_set_path = "dummy-queries.bin";
    std::string output_path = "output.bin";

    if (argc > 1) {
        database_path = std::string(args[1]);
    }

    if (argc > 2) {
        query_set_path = std::string(args[2]);
    }

    if (argc > 3) {
        output_path = std::string(args[3]);
    }
    
    Database database = ReadDatabase(database_path);
    std::cout << "Read database, length = " << database.length << std::endl;
    QuerySet query_set = ReadQuerySet(query_set_path);
    std::cout << "Read query_set, length = " << query_set.length << std::endl;

    // StatsQuerySet(query_set);
    // StatsDatabase(database);

    std::map<float32_t, std::pair<uint32_t, uint32_t>> C_map;
    IndexDatabase(database, C_map);
    SolveForQueries(output_path, database, C_map, query_set);

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
