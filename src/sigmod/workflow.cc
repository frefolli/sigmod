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

inline void PushCandidate(Scoreboard& scoreboard, Record& record, Query& query, uint32_t record_index) {
    scoreboard.emplace(Candidate({
        .index = record_index,
        .score = distance(query, record)
    }));
}

enum query_t {
    NORMAL = 0,
    BY_C = 1,
    BY_T = 2,
    BY_C_AND_T = 3
};

inline void FilterIndexesByT(Database& database, uint32_t& start_index, uint32_t& end_index, float32_t l, float32_t r) {
    // it's guaranteed that the database is ordered by C, T, fields
    // in such "order"
    start_index = SeekHigh(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, l
    );

    end_index = SeekLow(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, r
    );
}

inline void FilterIndexesByC(std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map, uint32_t& start_index, uint32_t& end_index, float32_t v) {
    start_index = C_map[v].first;
    end_index = C_map[v].second + 1;
}

inline void ExaustiveSearchByT(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        if (elegible_by_T(query, database.records[i])) {
            PushCandidate(scoreboard, database.records[i], query, i);
            if (scoreboard.size() > k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    }
}

inline void ExaustiveSearch(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        PushCandidate(scoreboard, database.records[i], query, i);
        if (scoreboard.size() > k_nearest_neighbors) {
            scoreboard.pop();
        }
    }
}

void FindForQuery(Result& result, Database& database, std::map<float32_t, std::pair<uint32_t, uint32_t>>& C_map, Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard(compare_function);

    const uint32_t query_type = (uint32_t) (query.query_type);

    uint32_t start_index = 0;
    uint32_t end_index = database.length;

    switch(query_type) {
        case BY_T: {
            for (auto C_it : C_map) {
                start_index = C_it.second.first;
                end_index = C_it.second.second;
                FilterIndexesByT(database, start_index, end_index, query.l, query.r);
                ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            }
            break;
        }; 
        case BY_C: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case BY_C_AND_T: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            FilterIndexesByT(database, start_index, end_index, query.l, query.r);
            ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case NORMAL: {
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
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
        #ifdef STOP_AFTER_1000
        if (i > 1000)
            break;
        #endif
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
