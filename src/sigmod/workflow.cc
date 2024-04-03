#include <sigmod/workflow.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <sigmod/kd_tree.hh>
#include <sigmod/ball_tree.hh>
#include <sigmod/exaustive.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>
#include <cstdint>

Solution SolveForQueriesWithExaustive(const Database& database,
                                      const c_map_t& C_map,
                                      const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        SearchExaustive(database, C_map, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithKDForest(const Database& database,
                                     const KDForest& forest,
                                     const c_map_t& C_map,
                                     const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        SearchKDForest(forest, database, C_map, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithBallForest(const Database& database,
                                       const BallForest& forest,
                                       const c_map_t& C_map,
                                       const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        SearchBallForest(forest, database, C_map, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

void Workflow(const std::string database_path,
              const std::string query_set_path,
              const std::string output_path) {
    Database database = ReadDatabase(database_path);
    LogTime("Read database, length = " + std::to_string(database.length));
    QuerySet query_set = ReadQuerySet(query_set_path);
    LogTime("Read query_set, length = " + std::to_string(query_set.length));

    c_map_t C_map;
    IndexDatabase(database, C_map);
    LogTime("Indexes Database");

    /* Initialization */
    #ifdef ENABLE_BALL_FOREST
    BallForest ball_forest = BuildBallForest(database, C_map);
    LogTime("Built Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    KDForest kd_forest = BuildKDForest(database, C_map);
    LogTime("Built KD Forest");
    #endif

    /* Usage */
    #ifdef ENABLE_BALL_FOREST
    Solution ball_forest_solution = SolveForQueriesWithBallForest(database, ball_forest, C_map, query_set);
    LogTime("Used Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    Solution kd_forest_solution = SolveForQueriesWithKDForest(database, kd_forest, C_map, query_set);
    LogTime("Used KD Forest");
    #endif

    #ifdef ENABLE_EXAUSTIVE
    Solution exaustive = SolveForQueriesWithExaustive(database, C_map, query_set);
    LogTime("Used Exaustive");
    #endif

    /* Free Solution */
    #ifdef ENABLE_BALL_FOREST
    FreeSolution(ball_forest_solution);
    LogTime("Freed Ball Forest Solution");
    #endif

    #ifdef ENABLE_KD_FOREST
    FreeSolution(kd_forest_solution);
    LogTime("Freed KD Forest Solution");
    #endif

    #ifdef ENABLE_EXAUSTIVE
    FreeSolution(exaustive);
    LogTime("Freed Exaustive Solution");
    #endif

    /* Free Models */
    #ifdef ENABLE_BALL_FOREST
    FreeBallForest(ball_forest);
    LogTime("Freed Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    FreeKDForest(kd_forest);
    LogTime("Freed KD Forest");
    #endif

    FreeDatabase(database);
    FreeQuerySet(query_set);
    LogTime("Freed DB&QS");
}
