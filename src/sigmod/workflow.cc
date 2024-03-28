#include <sigmod/workflow.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <queue>
#include <vector>
#include <map>

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
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

inline void PushCandidate(Scoreboard& scoreboard, Database& database, Query& query, uint32_t record_index) {
    scoreboard.emplace(Candidate({
        .index = record_index,
        .score = distance(query, database.records[record_index])
    }));
}

enum query_t {
    NORMAL = 0,
    BY_C = 1,
    BY_T = 2,
    BY_C_AND_T = 3
};

void FindForQuery(Result& result, Database& database, std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map, Query& query) {
    const uint32_t query_type = (uint32_t) (query.query_type);

    uint32_t start_index = 0;
    uint32_t end_index = database.length;

    if (query_type == BY_C || query_type == BY_C_AND_T) {
        start_index = C_map[query.v].first;
        end_index = C_map[query.v].second + 1;
    }

    if (query_type == BY_C_AND_T) {
        // it's guaranteed that the database is ordered by C, T, fields
        // in such "order"
        start_index = SeekHigh(
            [&database](uint32_t i) { return database.records[i].T; },
            start_index, end_index, query.l
        );

        end_index = SeekLow(
            [&database](uint32_t i) { return database.records[i].T; },
            start_index, end_index, query.r
        );
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

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}

Solution SolveForQueries(Database& database,
                         std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map,
                         QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        FindForQuery(solution.results[i], database, C_map, query_set.queries[i]);
    }
    return solution;
}

void Workflow(std::string database_path,
              std::string query_set_path,
              std::string output_path) {
    Database database = ReadDatabase(database_path);
    std::cout << "Read database, length = " << database.length << std::endl;
    QuerySet query_set = ReadQuerySet(query_set_path);
    std::cout << "Read query_set, length = " << query_set.length << std::endl;

    std::map<float32_t, std::pair<uint32_t, uint32_t>> C_map;
    IndexDatabase(database, C_map);

    Solution solution = SolveForQueries(database, C_map, query_set);
    WriteSolution(solution, output_path);

    FreeSolution(solution);
    FreeDatabase(database);
    FreeQuerySet(query_set);
}
