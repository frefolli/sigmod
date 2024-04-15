#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>
#include <random>
#include <cmath>
#include <chrono>

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
        dist += matr_dist[i][cb.vector_centroid[index_vector][i]];
    }

    return (const score_t) dist;
}

uint16_t** MallocVectorCentroid(const uint32_t db_length, const uint8_t n_partitions){
    uint16_t** vector_centroid = smalloc<uint16_t*>(db_length);
    for (uint32_t i = 0; i < db_length; i++) {
        vector_centroid[i] = smalloc<uint16_t>(n_partitions);
        for (uint8_t j = 0; j < n_partitions; j++) {
            vector_centroid[i][j] = 0;
        }
    }
    return vector_centroid;
}

void FreeVectorCentroid(CodeBook& cb) {
    if (cb.vector_centroid != nullptr) {    
        free(cb.vector_centroid);
    }
    cb.vector_centroid = nullptr;
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
    Debug("coso");

    std::map<uint16_t, float32_t[M]> &centroids = cb.centroids[n_partition];
    
    // Initializing centroids random on a point
    uint32_t ind_init_db = 0;
    for (uint32_t i = 0; i < K; i++) {
        ind_init_db = uni(rd);
        for (uint32_t j = 0; j < dim_partition; j++) {
            centroids[i][j] = database.records[ind_init_db].fields[j + start_partition_id];
        }
    }

    for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
        for (uint32_t i = 0; i < K; i++) {
            dim_centroid[i] = 0;
        }

        // FULL ITERATION
        // computing the nearest centroid
        for (uint32_t i = 0; i < database.length; i++) {
            Record& record = database.records[i];
            score_t min_dist = distance(centroids[0], record.fields, start_partition_id, end_partition_id);
            cb.vector_centroid[i][n_partition] = 0;
            dim_centroid[0]++;
            uint32_t anchored_centroid = 0;
            if (i % 10000 == 0)
            {
                Debug("i := " + std::to_string(i));
                /* code */
            }
            
            for (uint32_t j = 1; j < K; j++) {
                score_t dist = distance(centroids[j], record.fields, start_partition_id, end_partition_id);
                if (dist < min_dist) {
                    min_dist = dist; 
                    dim_centroid[anchored_centroid]--;
                    dim_centroid[j]++;
                    cb.vector_centroid[i][n_partition] = j;
                    anchored_centroid = j;
                }
            }
        }
        Debug("bene");

        std::map<uint16_t, float32_t [10]> old_centroids = centroids;

        // reset centroid
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                centroids[i][j] = 0;
            }
        }

        // refill centroid data
        for (uint32_t i = 0; i < database.length; i++) {
            uint32_t centroid = cb.vector_centroid[i][n_partition];
            Record& record = database.records[i];
            for (uint32_t j = 0; j < dim_partition; j++) {
                centroids[centroid][j] += record.fields[j + start_partition_id];
            }
        }
        
        // compute mean of cumulated coordinates
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                if (dim_centroid[i] != 0){
                    centroids[i][j] /= dim_centroid[i];
                } 
            }
        }

        Debug(" -- Iteration " + std::to_string(iteration) + " -- ");
        score_t cerr = 0;
        score_t err;
        for (uint32_t i = 0; i < K; i++) {
            err = 0;
            for (uint32_t j = 0; j < dim_partition; j++) {
                err += pow(centroids[i][j] - old_centroids[i][j], 2);
            }
            err = sqrt(err);
            cerr += err;    
            //Debug(" Centroid " + std::to_string(i) + " error := " + std::to_string(err));
        }
        Debug(" Cumulative centroid error := " + std::to_string(cerr));
        //compute_distributions(dim_centroid);

    }
    
}

void SearchExaustivePQ(const CodeBook& cb, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;
    score_t matr_dist[M][K];

    //assert(query.query_type == NORMAL);
    
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
