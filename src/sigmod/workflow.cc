#include <sigmod/workflow.hh>
#include <sigmod/flags.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <sigmod/kd_tree.hh>
#include <sigmod/ball_tree.hh>
#include <sigmod/vp_tree.hh>
#include <sigmod/exaustive.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>

Solution SolveForQueriesWithExaustive(const Database& database,
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
        SearchExaustive(database, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithKDForest(const Database& database,
                                     const KDForest& forest,
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
        SearchKDForest(forest, database, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithBallForest(const Database& database,
                                       const BallForest& forest,
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
        SearchBallForest(forest, database, solution.results[i], query_set.queries[i]);
    }
    return solution;
}

Solution SolveForQueriesWithVPForest(const Database& database,
                                     const VPForest& forest,
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
        SearchVPForest(forest, database, solution.results[i], query_set.queries[i]);
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

    IndexDatabase(database);
    LogTime("Indexes Database");

    /* Initialization */
    #ifdef ENABLE_BALL_FOREST
    BallForest ball_forest = BuildBallForest(database);
    LogTime("Built Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    KDForest kd_forest = BuildKDForest(database);
    LogTime("Built KD Forest");
    #endif

    #ifdef ENABLE_VP_FOREST
    VPForest vp_forest = BuildVPForest(database);
    LogTime("Built VP Forest");
    #endif

    /* Usage */
    #ifdef ENABLE_BALL_FOREST
    Solution ball_forest_solution = SolveForQueriesWithBallForest(database, ball_forest, query_set);
    LogTime("Used Ball Forest");
    #endif

    #ifdef ENABLE_KD_FOREST
    Solution kd_forest_solution = SolveForQueriesWithKDForest(database, kd_forest, query_set);
    LogTime("Used KD Forest");
    #endif

    #ifdef ENABLE_VP_FOREST
    Solution vp_forest_solution = SolveForQueriesWithVPForest(database, vp_forest, query_set);
    LogTime("Used VP Forest");
    #endif

    #ifdef ENABLE_EXAUSTIVE
    Solution exaustive_solution = SolveForQueriesWithExaustive(database, query_set);
    LogTime("Used Exaustive");
    #endif

    /* Comparison */
    #ifdef ENABLE_EXAUSTIVE
        #ifdef ENABLE_BALL_FOREST
            score_t ball_forest_score = CompareSolutions(database, query_set, exaustive_solution, ball_forest_solution);
            Debug("Recall(Ball Forest) := " + std::to_string(ball_forest_score));
            LogTime("Compared Ball Forest to Exaustive");
        #endif
        #ifdef ENABLE_KD_FOREST
            score_t kd_forest_score = CompareSolutions(database, query_set, exaustive_solution, kd_forest_solution);
            Debug("Recall(KD Forest) := " + std::to_string(kd_forest_score));
            LogTime("Compared KD Forest to Exaustive");
        #endif
        #ifdef ENABLE_VP_FOREST
            score_t vp_forest_score = CompareSolutions(database, query_set, exaustive_solution, vp_forest_solution);
            Debug("Recall(VP Forest) := " + std::to_string(vp_forest_score));
            LogTime("Compared VP Forest to Exaustive");
        #endif
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

    #ifdef ENABLE_VP_FOREST
    FreeSolution(vp_forest_solution);
    LogTime("Freed VP Forest Solution");
    #endif

    #ifdef ENABLE_EXAUSTIVE
    FreeSolution(exaustive_solution);
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

    #ifdef ENABLE_VP_FOREST
    FreeVPForest(vp_forest);
    LogTime("Freed VP Forest");
    #endif

    Debug("Remember that solution must be converted in the end by taking `i := database.records[i].index`");

    FreeDatabase(database);
    FreeQuerySet(query_set);
    LogTime("Freed DB&QS");
}
