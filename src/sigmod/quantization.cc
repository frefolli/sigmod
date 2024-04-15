#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>
#include <random>
#include <chrono>
/*
inline score_t distance(std::vector<float32_t>& centroid, const float32_t* record, const uint32_t start_index_field, const uint32_t end_partition_id) {
    #ifdef TRACK_DISTANCE_COMPUTATIONS
        SIGMOD_DISTANCE_COMPUTATIONS++;
    #endif
    score_t sum = 0;
    for (uint32_t i = 0; i <= end_partition_id - start_index_field + 1; i++) {
        score_t m = centroid[i] - record[i + start_index_field];
        sum += (m * m);
    }
    #ifdef FAST_DISTANCE
        return sum;
    #else
        #ifndef FAST_SQRT
            return std::sqrt(sum);
        #else
            return quacke3_sqrt(sum);
        #endif
    #endif
}*/

void PreprocessingQuery(score_t matr_dist[M][K], const float32_t* query, const CodeBook& cb){
    #pragma omp parallel for collapse(2)
    for (uint8_t i = 0; i < M; i++){
        // #pragma omp parallel for
        for (uint32_t j = 0; j < K; j++) {
            matr_dist[i][j]  = distance(cb.centroids.at(i).at(j), query, i * M, i * M + M - 1);
        }
    }
}

const score_t ADC(const score_t matr_dist[M][K], const CodeBook& cb, const uint32_t index_vector){
    score_t dist = 0;

    // #pragma omp parallel for
    for (uint8_t i = 0; i < M; i++) {
        dist += matr_dist[i][cb.vector_centroid.at(index_vector)[i]];
    }

    return (const score_t) dist;
}

/* [start_partition_id, end_partition_id] */

void Kmeans(
        CodeBook& cb,
        const Database& database, 
        const uint32_t ITERATIONS, 
        const uint32_t start_partition_id, 
        const uint32_t end_partition_id) {

    uint8_t n_partition = start_partition_id / M;
    
    std::vector<uint32_t> dim_centroid(K);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, database.length-1);
    // Initializing centroids random on a point
    uint32_t ind_init_db = 0;
    for (uint32_t i = 0; i < K; i++) {
        ind_init_db = uni(rd);
        for (uint32_t j = 0; j < dim_partition; j++) {
            cb.centroids[n_partition][i][j] = database.records[ind_init_db].fields[j + start_partition_id];
            dim_centroid[i] = 0;
        }
    }

    for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
        // FULL ITERATION
        // computing the nearest centroid
        for (uint32_t i = 0; i < database.length; i++) {
            Record& record = database.records[i];
            score_t min_dist = distance(cb.centroids[n_partition][0], record.fields, start_partition_id, end_partition_id);
            cb.vector_centroid[i][n_partition] = 0;
            dim_centroid[0]++;
            uint32_t anchored_centroid = 0;
            for (uint32_t j = 1; j < K; j++) {
                score_t dist = distance(cb.centroids[n_partition][j], record.fields, start_partition_id, end_partition_id);
                if (dist < min_dist) {
                    min_dist = dist;
                    dim_centroid[anchored_centroid]--;
                    dim_centroid[j]++;
                    cb.vector_centroid[i][n_partition] = j;
                    anchored_centroid = j;
                }
            }
        }
        // reset centroid
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                cb.centroids[n_partition][i][j] = 0;
            }
        }
        // refill centroid data
        for (uint32_t i = 0; i < database.length; i++) {
            uint32_t centroid = cb.vector_centroid[i][n_partition];
            Record& record = database.records[i];
            for (uint32_t j = 0; j < dim_partition; j++) {
                cb.centroids[n_partition][i][j] += record.fields[j + start_partition_id];
            }
        }
        // compute mean of cumulated coordinates
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                cb.centroids[n_partition][i][j] /= dim_centroid[i];
            }
        }
        
        Debug(" -- Iteration " + std::to_string(iteration) + " -- ");
        compute_distributions(dim_centroid);

    }
    
}

void SearchExaustivePQ(const CodeBook& cb, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;
    score_t matr_dist[M][K];

    assert(query.query_type == NORMAL);
    
    auto start_query_timer = std::chrono::high_resolution_clock::now();
    PreprocessingQuery(matr_dist, query.fields, cb);
    auto end_query_timer = std::chrono::high_resolution_clock::now();
        
    long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();
    Debug("TIME preprocessing (ms) := " + std::to_string(sample));

    start_query_timer = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < database.length; i++) {
        gboard.push(i, ADC(matr_dist, cb, i));
    }
    end_query_timer = std::chrono::high_resolution_clock::now();
        
    sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();
    Debug("TIME searching ADC (ms) := " + std::to_string(sample));

    assert(gboard.full());
    
    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        result.data[rank] = gboard.top().index;
        gboard.pop();
        rank--;
    }
}
