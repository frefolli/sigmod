#include <sigmod/workflow.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <sigmod/kd_tree.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

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

inline void FilterIndexesByC(c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, float32_t v) {
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

void FindForQuery(Result& result, Database& database, c_map_t& C_map, Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

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
                         c_map_t& C_map,
                         QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        FindForQuery(solution.results[i], database, C_map, query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithKDTree(Database& database,
                                   KDTree& tree,
                                   QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        KDTreeSearch2(solution.results[i], tree, database, query_set.queries[i]);
    }
    return solution;
}

void Statistics(Database& database) {
    const uint32_t N = vector_num_dimension;

    float32_t* xys = (float32_t*) malloc (N * N * sizeof(float32_t));
    float32_t* xs = (float32_t*) malloc (N * sizeof(float32_t));
    memset(xys, 0, N * N * sizeof(float32_t));
    memset(xs, 0, N * sizeof(float32_t));
    for (uint32_t k = 0; k < database.length; k++) {
        for (uint32_t i = 0; i < N; i++) {
            xs[i] += database.records[k].fields[i] / database.length;
            for (uint32_t j = i; j < N; j++) {
                const float32_t xy = (database.records[k].fields[i] * database.records[k].fields[j]) / database.length;
                xys[i * N + j] += xy;
                xys[j * N + i] += xy;
            }
        }
    }

    float32_t* means = (float32_t*) malloc (N * sizeof(float32_t));
    memset(means, 0, N * sizeof(float32_t));
    for (uint32_t i = 0; i < N; i++) {
        means[i] = xs[i];
    }

    float32_t* covariances = (float32_t*) malloc (N * N * sizeof(float32_t));
    memset(covariances, 0, N * N * sizeof(float32_t));
    for (uint32_t i = 0; i < N; i++) {
        for (uint32_t j = i; j < N; j++) {
            const float32_t cov = xys[i * N + j] - (means[i] * means[j]);
            covariances[i * N + j] = cov;
            covariances[j * N + i] = cov;
        }
    }

    float32_t* correlations = (float32_t*) malloc (N * N * sizeof(float32_t));
    memset(correlations, 0, N * N * sizeof(float32_t));
    for (uint32_t i = 0; i < N; i++) {
        for (uint32_t j = i; j < N; j++) {
            const float32_t corr = covariances[i * N + j] / sqrt(covariances[i * N + i] * covariances[j * N + j]);
            correlations[i * N + j] = corr;
            correlations[j * N + i] = corr;
        }
    }

    std::cout << "import numpy as np" << std::endl;
    std::cout << "def data():" << std::endl;
    std::cout << "\treturn np.array([" << std::endl;
    for (uint32_t i = 0; i < N; i++) {
        if (i != 0)
            std::cout << ",\n";
        std::cout << "\t[";
        for (uint32_t j = 0; j < N; j++) {
            if (j != 0)
                std::cout << ", ";
            std::cout << covariances[i * N + j];
        }
        std::cout << "]";
    }
    std::cout << "])" << std::endl;

    free(xys);
    free(xs);
    free(correlations);
    free(covariances);
    free(means);
}

void Workflow(std::string database_path,
              std::string query_set_path,
              std::string output_path) {
    Database database = ReadDatabase(database_path);
    std::cout << "# Read database, length = " << database.length << std::endl;
    QuerySet query_set = ReadQuerySet(query_set_path);
    std::cout << "# Read query_set, length = " << query_set.length << std::endl;

    c_map_t C_map;
    IndexDatabase(database, C_map);

    /*
    Statistics(database);
    
    KDTree tree = BuildKDTree(database);
    Debug("Built KDTree");
    */
    
    Solution exaustive = SolveForQueries(database, C_map, query_set);
    /*
    Solution kdtree = SolveForQueriesWithKDTree(database, tree, query_set);
    CompareSolutions(database, query_set, exaustive, kdtree);
    */

    WriteSolution(exaustive, output_path);
    FreeSolution(exaustive);

    /*
    FreeSolution(kdtree);
    FreeKDTree(tree);
    */

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
