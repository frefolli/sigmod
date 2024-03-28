#include <sigmod.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <queue>
#include <vector>

inline bool elegible_by_C(const Query& query, const Record& record) {
    return (query.v == record.C);
}

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

inline bool RecordIsElegible(const Query& query, const Record& record) {
    const uint32_t query_type = (uint32_t) (query.query_type);
    switch(query_type) {
        case 0: {
            return true;
        };
        case 1: {
            return elegible_by_C(query, record);
        };
        case 2: {
            return elegible_by_T(query, record);
        };
        case 3: {
            return (elegible_by_C(query, record) && elegible_by_T(query, record));
        };
    }
    return true;
}

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

void FindForQuery(FILE* solution, Database& database, Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard(compare_function);

    for (uint32_t i = 0; i < k_nearest_neighbors; i++) {
        if (RecordIsElegible(query, database.records[i])) {
            scoreboard.emplace(Candidate({
                .index = i,
                .score = distance(query, database.records[i])
            }));
        }
    }

    for (uint32_t i = k_nearest_neighbors; i < database.length; i++) {
        if (RecordIsElegible(query, database.records[i])) {
            scoreboard.emplace(Candidate({
                .index = i,
                .score = distance(query, database.records[i])
            }));
            if (scoreboard.size() > k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    }

    OutputSolution(solution, scoreboard);
}

void SolveForQueries(std::string output_path, Database& database, QuerySet& query_set) {
    FILE* solution = fopen(output_path.c_str(), "wb");
    for (uint32_t i = 0; i < query_set.length; i++) {
        FindForQuery(solution, database, query_set.queries[i]);
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

    StatsQuerySet(query_set);
    StatsDatabase(database);
    // IndexDatabase(database);
    // SolveForQueries(output_path, database, query_set);

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
