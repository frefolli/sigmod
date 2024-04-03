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

Solution SolveForQueriesWithExaustive(Database& database,
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
        SearchExaustive(solution.results[i], database, C_map, query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithKDForest(Database& database,
                                     KDForest& forest,
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
        SearchKDForest(forest, database, C_map, query_set.queries[i]);
    }
    return solution;
}

void SolveForQueriesWithBallForest(Database& database,
                                     BallForest& forest,
                                     c_map_t& C_map,
                                     QuerySet& query_set) {
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        SearchBallForest(forest, database, C_map, query_set.queries[i]);
    }
}

void Workflow(std::string database_path,
              std::string query_set_path,
              std::string output_path) {
    Database database = ReadDatabase(database_path);
    LogTime("Read database, length = " + std::to_string(database.length));
    QuerySet query_set = ReadQuerySet(query_set_path);
    LogTime("Read query_set, length = " + std::to_string(query_set.length));

    c_map_t C_map;
    IndexDatabase(database, C_map);
    LogTime("Indexes Database");

    #ifdef ENABLE_BALL_FOREST
    BallForest ball_forest = BuildBallForest(database, C_map);
    LogTime("Built Ball Forest");

    SolveForQueriesWithBallForest(database, ball_forest, C_map, query_set);
    LogTime("Used Ball Forest");

    FreeBallForest(ball_forest);
    LogTime("Freed Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    KDForest kd_forest = BuildKDForest(database, C_map);
    LogTime("Built KD Forest");

    SolveForQueriesWithKDForest(database, kd_forest, C_map, query_set);
    LogTime("Used KD Forest");

    FreeKDForest(kd_forest);
    LogTime("Freed KD Forest");
    #endif
    
    #ifdef ENABLE_EXAUSTIVE
    Solution exaustive = SolveForQueriesWithExaustive(database, C_map, query_set);
    LogTime("Used Exaustive");

    WriteSolution(exaustive, output_path);
    LogTime("Wrote Solution");

    FreeSolution(exaustive);
    #endif

    FreeDatabase(database);
    FreeQuerySet(query_set);
    LogTime("Freed DB&QS");
}
