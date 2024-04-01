#include <sigmod/workflow.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <sigmod/kd_tree.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fstream>

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

/*
 * x' = (x - mu) / sigma
 * */
struct Normative {
    float32_t* mus;
    float32_t* sigmas;
};

Normative NewNormative() {
    return {
        .mus = (float32_t*) malloc (sizeof(float32_t) * vector_num_dimension),
        .sigmas = (float32_t*) malloc (sizeof(float32_t) * vector_num_dimension)
    };
}

void FreeNormative(Normative& normative) {
    free(normative.mus);
    free(normative.sigmas);
}

template <typename WithFields>
ipernorm_t NormatedNorm(Normative& normative, WithFields& obj_with_fields) {
    ipernorm_t sum = 0;
    ipernorm_t pivot = 1;
    for (uint32_t i = vector_num_dimension - 1; i < vector_num_dimension; i--) {
        sum += pivot * (obj_with_fields.fields[i] - normative.mus[i]) / normative.sigmas[i];
        pivot *= 2;
    }
    return sum;
}

void Statistics(Database& database, QuerySet& query_set) {
    Normative normative = NewNormative();

    float32_t* maxs = (float32_t*) malloc (sizeof(float32_t) * vector_num_dimension);
    float32_t* mins = (float32_t*) malloc (sizeof(float32_t) * vector_num_dimension);
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        maxs[i] = database.records[0].fields[i];
        mins[i] = database.records[0].fields[i];
    }

    for (uint32_t i = 1; i < database.length; i++) {
        for (uint32_t j = 0; j < vector_num_dimension; j++) {
            const float32_t val = database.records[i].fields[j];
            if (maxs[j] < val)
                maxs[j] = val;
            if (mins[j] > val)
                mins[j] = val;
        }
    }
    
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        normative.mus[i] = mins[i];
        normative.sigmas[i] = (maxs[i] - mins[i]);
    }

    free(maxs);
    free(mins);

    ipernorm_t* db_norms = (ipernorm_t*) malloc (sizeof(ipernorm_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        db_norms[i] = NormatedNorm(normative, database.records[i]);
    }
    Debug("Computed DB Norms");

    uint32_t* db_indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        db_indexes[i] = i;
    }
    Debug("Build DB Norm Index");
 
    const auto compare_indexes_by_norm = [&db_norms](uint32_t a, uint32_t b) {
        return db_norms[a] < db_norms[b];
    };

    std::sort(db_indexes,
              db_indexes + database.length,
              compare_indexes_by_norm);
    Debug("Sorted DB Norm Index");

    ipernorm_t* qs_norms = (ipernorm_t*) malloc (sizeof(ipernorm_t) * query_set.length);
    for (uint32_t i = 0; i < query_set.length; i++) {
        qs_norms[i] = NormatedNorm(normative, query_set.queries[i]);
    }
    Debug("Computed QS Norms");

    auto p = SeekBoth(
        [&db_norms](uint32_t i) { return db_norms[i]; },
        0, database.length, qs_norms[0]
    );
    std::cout << "SB[QS[0]].f = " << p.first << std::endl;
    std::cout << "SB[QS[0]].s = " << p.second << std::endl;

    free(db_indexes);
    free(db_norms);
    free(qs_norms);
    FreeNormative(normative);
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
    Debug("Built C_Map");

    Statistics(database, query_set);
    Debug("Built Stats");

    /*
    KDTree tree = BuildKDTree(database);
    */
    
    /*
    Solution exaustive = SolveForQueries(database, C_map, query_set);
    Solution kdtree = SolveForQueriesWithKDTree(database, tree, query_set);
    CompareSolutions(database, query_set, exaustive, kdtree);

    WriteSolution(exaustive, output_path);
    FreeSolution(exaustive);

    FreeSolution(kdtree);
    FreeKDTree(tree);
    */

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
