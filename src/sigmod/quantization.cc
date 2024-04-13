#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>
#include <random>

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
}

inline score_t distance(const float32_t* centroid, const float32_t* record, const uint32_t start_index_field, const uint32_t end_partition_id) {
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
}


/* [start_partition_id, end_partition_id] */

void Kmeans(
        CodeBook& cb,
        const Database& database, 
        const uint32_t ITERATIONS, 
        const uint32_t start_partition_id, 
        const uint32_t end_partition_id) {

    uint8_t n_partition = start_partition_id / M;

    //const uint32_t dimension_partition = end_partition_id - start_partition_id + 1;
    //std::vector<uint32_t> beholds(database.length);

    std::vector<uint32_t> dim_centroid(K);

    //std::vector<std::vector<float32_t>> centroids(k);
    //for (uint32_t i = 0; i < k; i++)
    //{
    //    centroids[i] = std::vector<float32_t>(dimension_partition);
    //}
    
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, database.length-1);

    Debug("waaaa"); 
    // Initializing centroids random on a point
    uint32_t ind_init_db = 0;
    for (uint32_t i = 0; i < K; i++) {
        ind_init_db = uni(rd);
        for (uint32_t j = 0; j < M / database.length; j++) {
            //centroids[i][j] = database.records[ind_init_db].fields[j + start_partition_id];
            cb.centroids[n_partition][i][j] = database.records[ind_init_db].fields[j + start_partition_id];
            dim_centroid[i] = 0;
        }
    }

    Debug("waaaa"); 

    for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
        // FULL ITERATION
        // computing the nearest centroid
        for (uint32_t i = 0; i < database.length; i++) {
            Record& record = database.records[i];
            score_t min_dist = distance(cb.centroids[n_partition][0], record.fields, start_partition_id, end_partition_id);
            //beholds[i] = 0;
            cb.vector_centroid[i][n_partition] = 0;
            dim_centroid[0]++;
            uint32_t anchored_centroid = 0;
            for (uint32_t j = 1; j < K; j++) {
                score_t dist = distance(cb.centroids[n_partition][i], record.fields, start_partition_id, end_partition_id);
                if (dist < min_dist) {
                    min_dist = dist;
                    dim_centroid[anchored_centroid]--;
                    dim_centroid[j]++;
                    cb.vector_centroid[i][n_partition] = j;
                    anchored_centroid = j;
                }
            }
        }
    Debug("waaaa"); 

        // reset centroid
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < database.length / M; j++) {
                //centroids[i][j] = 0;
                cb.centroids[n_partition][i][j] = 0;
            }
        }


        // refill centroid data
        for (uint32_t i = 0; i < database.length; i++) {
            //uint32_t centroid = beholds[i];
            uint32_t centroid = cb.vector_centroid[i][n_partition];
            Record& record = database.records[i];
            for (uint32_t j = 0; j < database.length / M; j++) {
                //centroids[centroid][j] += record.fields[j + start_partition_id];
                cb.centroids[n_partition][i][j] += record.fields[j + start_partition_id];
            }
        }


        // compute mean of cumulated coordinates
        for (uint32_t i = 0; i < K; i++) {
            for (uint32_t j = 0; j < database.length / M; j++) {
                //centroids[i][j] /= dim_centroid[i];
                cb.centroids[n_partition][i][j] /= dim_centroid[i];
            }
        }

        Debug(" -- Iteration " + std::to_string(iteration) + " -- ");
        compute_distributions(dim_centroid);

    }
    /*
    score_t sum = 0;
    // print counts
    for (uint32_t i = 0; i < k; i++) {
        std::cout << "len(centroids["
                    << i << "]) = "
                    << dim_centroid[i] << std::endl;
        sum += dim_centroid[i];
    }

    float32_t mean =sum/k;

    Debug("Distribution of vectors between centroids");
    Debug("tot := " + std::to_string(sum));
    Debug("mean := " + std::to_string(mean));
    Debug("median := " + std::to_string(dim_centroid[k/2-1]));

    sum = 0;
    for (uint32_t i = 0; i < k; i++) {
        sum += pow(dim_centroid[i] -  mean, 2);
    }
    Debug("var := " + std::to_string(sum/k));
    Debug("std := " + std::to_string(sqrt(sum/k)));
*/  

    /*if (beholds != nullptr) {
        //free(beholds);
    }*/
}