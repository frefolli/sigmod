#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>
#include <random>
#include <cmath>
#include <chrono>

void PreprocessingQuery(score_t** matr_dist, const float32_t* query, const CodeBook& cb){
    #pragma omp parallel for collapse(2)
    for (uint16_t i = 0; i < cb.M; i++){
        // #pragma omp parallel for
        for (uint32_t j = 0; j < cb.K; j++) {
            matr_dist[i][j]  = distance(cb.codewords[i].centroids[j].data, query, i * cb.dim_partition, i * cb.dim_partition + cb.dim_partition - 1);
        }
    }
}

const score_t ADC(const score_t** matr_dist, const CodeBook& cb, const uint32_t index_vector){
    score_t dist = 0;

    // #pragma omp parallel for
    for (uint16_t i = 0; i < cb.M; i++) {
        dist += matr_dist[i][cb.index_vector_to_index_centroid[index_vector][i]];
    }

    return (const score_t) dist;
}

CodeBook& MallocCodeBook(const uint32_t db_length, const uint16_t K, const uint16_t M){
    CodeBook* cb = smalloc<CodeBook>();
    cb->K = K;
    cb->M = M;
    cb->dim_partition = actual_vector_size / M;
    cb->db_length = db_length;
    cb->index_vector_to_index_centroid = smalloc<uint16_t*>(cb->db_length);
    for (uint32_t i = 0; i < cb->db_length; i++) {
        cb->index_vector_to_index_centroid[i] = smalloc<uint16_t>(cb->M);
    }
    
    cb->codewords = smalloc<Codeword>(cb->M);
    for (uint16_t i = 0; i < M; i++){
        cb->codewords[i].centroids = smalloc<Centroid>(cb->K);
        for (uint32_t j = 0; j < K; j++) {
            cb->codewords[i].centroids[j].data = smalloc<float32_t>(cb->dim_partition);
        }
        
    }
    return *cb;
}

Codeword& CloneCodeword(const Codeword& cw, const uint16_t K, const uint16_t dim_partition){
    Codeword* cw_cp = smalloc<Codeword>();
    cw_cp->centroids = smalloc<Centroid>(K);
    for (uint32_t i = 0; i < K; i++) {
        cw_cp->centroids[i].data = smalloc<float32_t>(dim_partition);
        for (uint32_t j = 0; j < dim_partition; j++){
            cw_cp->centroids[i].data[j] = cw.centroids[i].data[j];
        }    
    }
     
    return *cw_cp;
}

void FreeCodeword(Codeword* codeword, const uint16_t K) {
    if (codeword != nullptr) {
        if (codeword->centroids != nullptr){
            for (uint16_t i = 0; i < K; i++) {
                free(codeword->centroids[i].data);
                codeword->centroids[i].data = nullptr;
            }
            free(codeword->centroids);
            codeword->centroids = nullptr;
        }
    }
}

void FreeCodeBook(CodeBook* cb) {
    if (cb != nullptr) {
        if (cb->index_vector_to_index_centroid != nullptr){
            for (uint32_t i = 0; i < cb->db_length; i++){
                free(cb->index_vector_to_index_centroid[i]);
            } 
            free(cb->index_vector_to_index_centroid);
            cb->index_vector_to_index_centroid = nullptr;
        }
        if (cb->codewords != nullptr){
            for (uint32_t i = 0; i < cb->M; i++){
                FreeCodeword(cb->codewords + i, cb->K);
            } 
            free(cb->codewords);
            cb->codewords = nullptr;
        }
    }
}


/* [start_partition_id, end_partition_id] */
void Kmeans(
        CodeBook& cb,
        const Database& database, 
        const uint32_t ITERATIONS, 
        const uint32_t start_partition_id, 
        //const uint32_t end_partition_id,
        const uint32_t length) {

    uint16_t n_partition = start_partition_id / cb.dim_partition;
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, length-1);
    score_t cerr = 1;

    Codeword &codeword = cb.codewords[n_partition];
    
    // Initializing centroids random on a point
    for (uint32_t i = 0; i < cb.K; i++) {
        uint32_t ind_init_db = uni(rd);
        for (uint32_t j = 0; j < cb.dim_partition; j++) {
            codeword.centroids[i].data[j] = database.records[ind_init_db].fields[j + start_partition_id];
        }
        codeword.centroids[i].n_vectors_mapped = 0;
    }

    for (uint32_t iteration = 0; iteration < ITERATIONS  && cerr > 10e-4; iteration++) {
        for (uint32_t i = 0; i < cb.K; i++) {
            codeword.centroids[i].n_vectors_mapped = 0;
        }

        // FULL ITERATION
        // computing the nearest centroid
        for (uint32_t i = 0; i < length; i++) {
            Record& record = database.records[i];
            score_t min_dist = distance(codeword.centroids[0].data, record.fields, start_partition_id, start_partition_id + cb.dim_partition - 1);
            cb.index_vector_to_index_centroid[i][n_partition] = 0;
            codeword.centroids[0].n_vectors_mapped++;
            uint32_t anchored_centroid = 0;
            
            for (uint32_t j = 1; j < cb.K; j++) {
                score_t dist = distance(codeword.centroids[j].data, record.fields, start_partition_id, start_partition_id + cb.dim_partition - 1);
                if (dist < min_dist) {
                    min_dist = dist; 
                    codeword.centroids[anchored_centroid].n_vectors_mapped--;
                    codeword.centroids[j].n_vectors_mapped++;
                    cb.index_vector_to_index_centroid[i][n_partition] = j;
                    anchored_centroid = j;
                }
            }
        }

        Codeword old_codeword = CloneCodeword(codeword, cb.K, cb.dim_partition);

        // reset centroid
        for (uint32_t i = 0; i < cb.K; i++) {
            for (uint32_t j = 0; j < cb.dim_partition; j++) {
                codeword.centroids[i].data[j] = 0;
            }
        }

        // refill centroid data
        for (uint32_t i = 0; i < length; i++) {
            uint32_t centroid = cb.index_vector_to_index_centroid[i][n_partition];
            Record& record = database.records[i];
            for (uint32_t j = 0; j < cb.dim_partition; j++) {
                codeword.centroids[centroid].data[j] += record.fields[j + start_partition_id];
            }
        }
        
        // compute mean of cumulated coordinates
        for (uint32_t i = 0; i < cb.K; i++) {
            for (uint32_t j = 0; j < cb.dim_partition; j++) {
                if (codeword.centroids[i].n_vectors_mapped != 0){
                    codeword.centroids[i].data[j] /= codeword.centroids[i].n_vectors_mapped;
                } 
            }
        }

        Debug(" -- Iteration " + std::to_string(iteration) + " -- ");
        cerr = 0;
        score_t err;
        for (uint32_t i = 0; i < cb.K; i++) {
            err = 0;
            for (uint32_t j = 0; j < cb.dim_partition; j++) {
                err += pow(codeword.centroids[i].data[j] - old_codeword.centroids[i].data[j], 2);
            }
            err = sqrt(err);
            cerr += err;    
        }
        Debug(" Cumulative centroid error := " + std::to_string(cerr));
        FreeCodeword(&old_codeword, 1); 
    }
}

void quantization(CodeBook& cb, const Database& database, const uint32_t ITERATIONS){
    #pragma omp parallel for
        for (uint32_t i = 0; i < cb.M; i++) {
            Kmeans(cb, database, ITERATIONS, i * cb.dim_partition, /*i * (cb.dim_partition) + cb.dim_partition - 1,*/ database.length);
        }
}

void SearchExaustivePQ(const CodeBook& cb, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;
    score_t** matr_dist = smalloc<score_t*>(cb.M);
    for (uint16_t i = 0; i < cb.M; i++){
        matr_dist[i] = smalloc<score_t>(cb.K);
    }
    //assert(query.query_type == NORMAL);

    auto start_query_timer = std::chrono::high_resolution_clock::now();
    PreprocessingQuery(matr_dist, query.fields, cb);
    auto end_query_timer = std::chrono::high_resolution_clock::now();
        
    long long sample = std::chrono::duration_cast<std::chrono::milliseconds>(end_query_timer - start_query_timer).count();
    //Debug("TIME preprocessing (ms) := " + std::to_string(sample));

    start_query_timer = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < database.length; i++) {
        gboard.push(i, ADC((const score_t**)matr_dist, cb, i));
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

    for (uint16_t i = 0; i < cb.M; i++){
        free(matr_dist[i]);
    }
    free(matr_dist);
}

score_t min_centroid(const score_t* vect, const uint32_t length){
    uint32_t min_idx = 0;
    score_t min = vect[0];
    
    for (uint32_t i = 1; i < length; i++) {
        if (vect[i] < min){
            min = vect[i];
            min_idx = i;
        }
    }
    return min_idx;
}

void DebugCodeBook(const CodeBook& cb){
    Debug("--Codebook data--");
    Debug("M := " + std::to_string(cb.M));
    Debug("K := " + std::to_string(cb.K));
    Debug("db_length := " + std::to_string(cb.db_length));
    Debug("dim_partition := " + std::to_string(cb.dim_partition));
    Debug("--Bilanciamento centroidi partizione 1-- ");
    uint32_t count = 0;
    for (uint32_t i = 0; i < cb.K; i++){
        count += cb.codewords[0].centroids[i].n_vectors_mapped;
        Debug(std::to_string(i) + "-centroid := " + std::to_string(cb.codewords[0].centroids[i].n_vectors_mapped));
    }
    Debug("Total elements := " + std::to_string(count));
}

void DebugQuantization(const CodeBook& cb, const Database& db){
    uint32_t count_err = 0;
    for (uint32_t i = 0; i < cb.db_length; i++) {
        score_t** matr_dist = smalloc<score_t*>(cb.M);
        for (uint32_t j = 0; j < cb.M; j++){
            matr_dist[j] = smalloc<score_t>(cb.K);
        }

        PreprocessingQuery(matr_dist, db.records[i].fields, cb);
        /*
        Debug("Distance matrix between vector and all centroids");
        for (uint32_t j = 0; j < cb.M; j++){
            for (uint32_t k = 0; k < cb.K; k++){
                std::cout << matr_dist[j][k] << " "; 
            }
            std::cout << std::endl; 
        }*/

        for (uint32_t j = 0; j < cb.M; j++){
            uint32_t centroid =  min_centroid((const score_t*)matr_dist[j], cb.K);
            if ( centroid != cb.index_vector_to_index_centroid[i][j]){
            
                /*for (uint32_t k = 0; k < cb.K; k++){
                    std::cout << matr_dist[j][k] << " "; 
                }
                std::cout << std::endl; */
            
                Debug("Wrong assignment vector to centroid: right centroid := " + std::to_string(centroid) + ", assigned centroid := "+ std::to_string(cb.index_vector_to_index_centroid[i][j]));
                count_err++;
            }
        }
        free(matr_dist);
    }
    Debug("error := " + std::to_string(count_err));
}