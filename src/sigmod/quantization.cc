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
            matr_dist[i][j]  = distance(cb.codewords[i].centroids[j].data, query, i * M, i * M + M - 1);
        }
    }
}

const score_t ADC(const score_t matr_dist[M][K], const CodeBook& cb, const uint32_t index_vector){
    score_t dist = 0;

    // #pragma omp parallel for
    for (uint8_t i = 0; i < M; i++) {
        dist += matr_dist[i][cb.vector_to_centroid[index_vector].centroids[i]];
    }

    return (const score_t) dist;
}

CodeBook& MallocCodeBook(const uint32_t db_length, const uint16_t K, const uint8_t M, const uint8_t dim_partition){
    CodeBook* cb = smalloc<CodeBook>();
    cb->vector_to_centroid = smalloc<VectorToCentroids>(db_length);
    Debug("ok");
    cb->codewords = smalloc<Codeword>(M);
    for (uint8_t i = 0; i < M; i++){
        Debug("1");
        cb->codewords[i].centroids = smalloc<Centroid>(K);
    }
    
    return *cb;
}

Codeword& CloneCodeword(const Codeword& cw){
    Codeword* cw_cp = smalloc<Codeword>();
    cw_cp->centroids = smalloc<Centroid>(K);
    for (uint32_t i = 0; i < K; i++) {
        for (uint32_t j = 0; j < dim_partition; j++){
            cw_cp->centroids[i].data[j] = cw.centroids[i].data[j];
        }    
    }
     
    return *cw_cp;
}

void FreeCodeBook(CodeBook* cb) {
    if (cb != nullptr) {
        if (cb->vector_to_centroid != nullptr) {
            free(cb->vector_to_centroid);
            cb->vector_to_centroid = nullptr;
        } 
        FreeCodeword(cb->codewords);
    }
}

void FreeCodeword(Codeword* cw) {
    if (cw != nullptr) {
        free(cw->centroids);
        cw->centroids = nullptr;
        //free(cw); // ! invalid pointer error
    }
}

/* [start_partition_id, end_partition_id] */
void Kmeans(
        CodeBook& cb,
        const Database& database, 
        const uint32_t ITERATIONS, 
        const uint32_t start_partition_id, 
        const uint32_t end_partition_id,
        const uint32_t length) {

    uint8_t n_partition = start_partition_id / M;
    
    uint32_t dim_centroid[K];
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, length-1);

    Codeword &codeword = cb.codewords[n_partition];
    
    // Initializing centroids random on a point
    #pragma omp parallel for
    for (uint32_t i = 0; i < K; i++) {
        uint32_t ind_init_db = uni(rd);
        for (uint32_t j = 0; j < dim_partition; j++) {
            codeword.centroids[i].data[j] = database.records[ind_init_db].fields[j + start_partition_id];
        }
    }

    for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
        #pragma omp parallel for
        for (uint32_t i = 0; i < K; i++) {
            dim_centroid[i] = 0;
        }

        // FULL ITERATION
        // computing the nearest centroid
        for (uint32_t i = 0; i < length; i++) {
            Record& record = database.records[i];
            score_t min_dist = distance(codeword.centroids[0].data, record.fields, start_partition_id, end_partition_id);
            cb.vector_to_centroid[i].centroids[n_partition] = 0;
            dim_centroid[0]++;
            uint32_t anchored_centroid = 0;
            
            for (uint32_t j = 1; j < K; j++) {
                score_t dist = distance(codeword.centroids[j].data, record.fields, start_partition_id, end_partition_id);
                if (dist < min_dist) {
                    min_dist = dist; 
                    dim_centroid[anchored_centroid]--;
                    dim_centroid[j]++;
                    cb.vector_to_centroid[i].centroids[n_partition] = j;
                    anchored_centroid = j;
                }
            }
        }

        Codeword old_codeword = CloneCodeword(codeword);

        // reset centroid
        #pragma omp parallel for
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                codeword.centroids[i].data[j] = 0;
            }
        }

        // refill centroid data
        #pragma omp parallel for
        for (uint32_t i = 0; i < length; i++) {
            uint32_t centroid = cb.vector_to_centroid[i].centroids[n_partition];
            Record& record = database.records[i];
            for (uint32_t j = 0; j < dim_partition; j++) {
                codeword.centroids[centroid].data[j] += record.fields[j + start_partition_id];
            }
        }
        
        // compute mean of cumulated coordinates
        #pragma omp parallel for
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < dim_partition; j++) {
                if (dim_centroid[i] != 0){
                    codeword.centroids[i].data[j] /= dim_centroid[i];
                } 
            }
        }

        Debug(" -- Iteration " + std::to_string(iteration) + " -- ");
        score_t cerr = 0;
        score_t err;
        for (uint32_t i = 0; i < K; i++) {
            err = 0;
            for (uint32_t j = 0; j < dim_partition; j++) {
                err += pow(codeword.centroids[i].data[j] - old_codeword.centroids[i].data[j], 2);
            }
            err = sqrt(err);
            cerr += err;    
        }
        Debug(" Cumulative centroid error := " + std::to_string(cerr));
        //compute_distributions(dim_centroid);
        FreeCodeword(&old_codeword); // ! error Invalid Pointer 
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
    //Debug("TIME preprocessing (ms) := " + std::to_string(sample));

    start_query_timer = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < database.length; i++) {
        gboard.push(i, ADC(matr_dist, cb, i));
    }
    end_query_timer = std::chrono::high_resolution_clock::now();
        
    sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();
    //Debug("TIME searching ADC (ms) := " + std::to_string(sample));

    assert(gboard.full());
    
    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        result.data[rank] = gboard.top().index;
        gboard.pop();
        rank--;
    }
}
