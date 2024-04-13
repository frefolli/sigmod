#include <sigmod/workflow.hh>
#include <sigmod/flags.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <sigmod/kd_tree.hh>
#include <sigmod/ball_tree.hh>
#include <sigmod/vp_tree.hh>
#include <sigmod/mvp_tree.hh>
#include <sigmod/exaustive.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>
#include <sigmod/random_projection.hh>
#include <chrono>
#include <sigmod/quantization.hh>

Solution SolveForQueriesWithExaustive(const Database& database,
                                      const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length),
    };

    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        uint8_t query_type = (uint8_t) query_set.queries[i].query_type;

        auto start_query_timer = std::chrono::high_resolution_clock::now();
        
        SearchExaustive(database, solution.results[i], query_set.queries[i]);
        
        auto end_query_timer = std::chrono::high_resolution_clock::now();
        
        long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();

        solution.time_score_queries[query_type].second = 
            solution.time_score_queries[query_type].first / (solution.time_score_queries[query_type].first + 1) * solution.time_score_queries[query_type].second 
            + sample / (solution.time_score_queries[query_type].first + 1);
    }

    return solution;
}

Solution SolveForQueriesWithKDForest(const Database& database,
                                     const KDForest& forest,
                                     const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length),
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        uint8_t query_type = (uint8_t) query_set.queries[i].query_type;

        auto start_query_timer = std::chrono::high_resolution_clock::now();
        
        SearchKDForest(forest, database, solution.results[i], query_set.queries[i]);
        
        auto end_query_timer = std::chrono::high_resolution_clock::now();
        
        long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();

        solution.time_score_queries[query_type].second = 
            solution.time_score_queries[query_type].first / (solution.time_score_queries[query_type].first + 1) * solution.time_score_queries[query_type].second 
            + sample / (solution.time_score_queries[query_type].first + 1);
    }

    return solution;
}

Solution SolveForQueriesWithBallForest(const Database& database,
                                       const BallForest& forest,
                                       const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length),
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif

        uint8_t query_type = (uint8_t) query_set.queries[i].query_type;

        auto start_query_timer = std::chrono::high_resolution_clock::now();
        
        SearchBallForest(forest, database, solution.results[i], query_set.queries[i]);
        
        auto end_query_timer = std::chrono::high_resolution_clock::now();
        
        long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();

        solution.time_score_queries[query_type].second = 
            solution.time_score_queries[query_type].first / (solution.time_score_queries[query_type].first + 1) * solution.time_score_queries[query_type].second 
            + sample / (solution.time_score_queries[query_type].first + 1);
    }

    return solution;
}

Solution SolveForQueriesWithVPForest(const Database& database,
                                     const VPForest& forest,
                                     const QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length),
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        uint8_t query_type = (uint8_t) query_set.queries[i].query_type;

        auto start_query_timer = std::chrono::high_resolution_clock::now();

        SearchVPForest(forest, database, solution.results[i], query_set.queries[i]);
        
        auto end_query_timer = std::chrono::high_resolution_clock::now();
        
        long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();

        solution.time_score_queries[query_type].second = 
            solution.time_score_queries[query_type].first / (solution.time_score_queries[query_type].first + 1) * solution.time_score_queries[query_type].second 
            + sample / (solution.time_score_queries[query_type].first + 1);
    
    }

    return solution;
}

Solution SolveForQueriesWithMVPForest(const Database& database,
                                      const MVPForest& forest,
                                      const QuerySet& query_set) {
    score_t* PATH = smalloc<score_t>(forest.max_p);
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        #ifdef STOP_AFTER_TOT_ELEMENTS
        if (i >= TOT_ELEMENTS)
            break;
        #endif
        MVPForest::Search(forest, database, PATH, solution.results[i], query_set.queries[i]);
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

    #ifdef ENABLE_DIM_REDUCTION
    const float32_t** prj_matrix = ReduceDimensionality(database, N_DIM_REDUCTION);
    LogTime("Dimensional reduction database");
    
    ReduceDimensionality(query_set, prj_matrix, N_DIM_REDUCTION);

    FreeProjectionMatrix((float32_t**) prj_matrix);
    LogTime("Dimensional reduction queryset");

    actual_vector_size = N_DIM_REDUCTION;
    #endif

    IndexDatabase(database);
    LogTime("Indexes Database");

    #ifdef ENABLE_PRODUCT_QUANTIZATION

    CodeBook codebook;
    //#pragma omp parallel for num_threads(10)
        for (uint32_t i = 0; i < 10; i++) {
            Kmeans(codebook, database, 1, i * M, i * M + M - 1);
        }

    LogTime("Clusterized Single portion of Database");
    #endif



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

    #ifdef ENABLE_MVP_FOREST
    MVPForest mvp_forest = MVPForest::Build(database);
    LogTime("Built MVP Forest");
    #endif

    /* Usage */
    #ifdef ENABLE_BALL_FOREST
    Solution ball_forest_solution = SolveForQueriesWithBallForest(database, ball_forest, query_set);
    LogTime("Used Ball Forest");
    for(uint8_t k = 0; k < 4; k++){
        std::pair<uint32_t, uint64_t> time_score_query = ball_forest_solution.time_score_queries[k];
        Debug("Tot. queries type " + std::to_string(k) + " := " + std::to_string(time_score_query.first) + ", executed in " + 
        std::to_string(time_score_query.second) + " ms/q");
    }
    #endif

    #ifdef ENABLE_KD_FOREST
    Solution kd_forest_solution = SolveForQueriesWithKDForest(database, kd_forest, query_set);
    LogTime("Used KD Forest");
    for(uint8_t k = 0; k < 4; k++){
        std::pair<uint32_t, uint64_t> time_score_query = kd_forest_solution.time_score_queries[k];
        Debug("Tot. queries type " + std::to_string(k) + " := " + std::to_string(time_score_query.first) + ", executed in " + 
        std::to_string(time_score_query.second) + " ms/q");
    }
    #endif

    #ifdef ENABLE_VP_FOREST
    Solution vp_forest_solution = SolveForQueriesWithVPForest(database, vp_forest, query_set);
    LogTime("Used VP Forest");
    for(uint8_t k = 0; k < 4; k++){
        std::pair<uint32_t, uint64_t> time_score_query = vp_forest_solution.time_score_queries[k];
        Debug("Tot. queries type " + std::to_string(k) + " := " + std::to_string(time_score_query.first) + ", executed in " + 
        std::to_string(time_score_query.second) + " ms/q");
    }
    #endif

    #ifdef ENABLE_MVP_FOREST
    Solution mvp_forest_solution = SolveForQueriesWithMVPForest(database, mvp_forest, query_set);
    LogTime("Used MVP Forest");
    #endif

    #ifdef ENABLE_EXAUSTIVE
    Solution exaustive_solution = SolveForQueriesWithExaustive(database, query_set);
    LogTime("Used Exaustive");
    for(uint8_t k = 0; k < 4; k++){
        std::pair<uint32_t, uint64_t> time_score_query = exaustive_solution.time_score_queries[k];
        Debug("Tot. queries type " + std::to_string(k) + " := " + std::to_string(time_score_query.first) + ", executed in " + 
        std::to_string(time_score_query.second) + " ms/q");
    }
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
        #ifdef ENABLE_MVP_FOREST
            score_t mvp_forest_score = CompareSolutions(database, query_set, exaustive_solution, mvp_forest_solution);
            Debug("Recall(MVP Forest) := " + std::to_string(mvp_forest_score));
            LogTime("Compared MVP Forest to Exaustive");
        #endif
    #endif

    /* Save Solution */
    #ifdef SAVE_SOLUTION
        std::string out_path;
        std::string suffix = "";
        #ifdef ENABLE_DIM_REDUCTION 
        #ifdef N_DIM_REDUCTION 
            suffix += "-red-" + std::to_string(N_DIM_REDUCTION) + "d";
        #endif
        #endif
        #ifdef ENABLE_EXAUSTIVE
            out_path = GenerateOutputPathFileName(output_path, "", suffix + "-exaustive");
            WriteSolution(exaustive_solution, out_path);
        #endif
        #ifdef ENABLE_BALL_FOREST
            out_path = GenerateOutputPathFileName(output_path, "", suffix + "-ball-forest");
            WriteSolution(ball_forest_solution, out_path);
        #endif
        #ifdef ENABLE_KD_FOREST
            out_path = GenerateOutputPathFileName(output_path, "", suffix + "-kd-forest");
            WriteSolution(kd_forest_solution, out_path);
        #endif
        #ifdef ENABLE_MVP_FOREST
            out_path = GenerateOutputPathFileName(output_path, "", suffix + "-mvp-forest");
            WriteSolution(mvp_forest_solution, out_path);
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

    #ifdef ENABLE_MVP_FOREST
    FreeSolution(mvp_forest_solution);
    LogTime("Freed MVP Forest Solution");
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

    #ifdef ENABLE_MVP_FOREST
    MVPForest::Free(mvp_forest);
    LogTime("Freed MVP Forest");
    #endif

    Debug("Remember that solution must be converted in the end by taking `i := database.records[i].index`");

    FreeDatabase(database);
    FreeQuerySet(query_set);
    LogTime("Freed DB&QS");
}
