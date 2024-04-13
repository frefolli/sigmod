#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>
#include <random>


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

float32_t* Kmeans(
        const Database& database, 
        const uint32_t ITERATIONS, 
        const uint32_t start_partition_id, 
        const uint32_t end_partition_id,  
        const uint32_t k) {
    //const uint32_t ITERATIONS = 1;
    const uint32_t dimension_partition = end_partition_id - start_partition_id + 1;
    uint32_t* beholds = smalloc<uint32_t>(database.length);
    uint32_t* dim_centroid = smalloc<uint32_t>(k);

    float32_t** centroids = smalloc<float32_t*>(k);
    for (uint32_t i = 0; i < k; i++) {
        
        centroids[i] = smalloc<float32_t>(dimension_partition);
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, database.length-1);
    uint32_t ind_init_db = 0;

    Debug("Memory occuped := " + std::to_string(SIGMOD_MEMORY_TRACKER));
    
    #pragma omp parallel
    {
        #pragma omp barrier
        // Initializing centroids random on a point
        #pragma omp for
        for (uint32_t i = 0; i < k; i++) {
            ind_init_db = uni(rd);
            for (uint32_t j = 0; j < dimension_partition; j++) {
                centroids[i][j] = database.records[ind_init_db].fields[j + start_partition_id];
                dim_centroid[i] = 0;
            }
        }

        #pragma omp barrier

        for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
            // FULL ITERATION
            // computing the nearest centroid
            #pragma omp for
            for (uint32_t i = 0; i < database.length; i++) {
                Record& record = database.records[i];
                score_t min_dist = distance(centroids[0], record.fields, start_partition_id, end_partition_id);
                beholds[i] = 0;
                dim_centroid[0]++;
                uint32_t anchored_centroid = 0;
                for (uint32_t j = 1; j < k; j++) {
                    score_t dist = distance(centroids[j], record.fields, start_partition_id, end_partition_id);
                    if (dist < min_dist) {
                        min_dist = dist;
                        dim_centroid[anchored_centroid]--;
                        dim_centroid[j]++;
                        beholds[i] = j;
                        anchored_centroid = j;
                    }
                }
            }

            #pragma omp barrier

            // reset centroid
            #pragma omp for
            for (uint32_t i = 0; i < k; i++) {
                for (uint32_t j = 0; j < dimension_partition; j++) {
                    centroids[i][j] = 0;
                }
            }

            #pragma omp barrier

            // refill centroid data
            #pragma omp for
            for (uint32_t i = 0; i < database.length; i++) {
                uint32_t centroid = beholds[i];
                Record& record = database.records[i];
                #pragma omp critical
                {
                    for (uint32_t j = 0; j < dimension_partition; j++) {
                        centroids[centroid][j] += record.fields[j + start_partition_id];
                    }
                }
            }

            #pragma omp barrier

            // compute mean of cumulated coordinates
            #pragma omp for
            for (uint32_t i = 0; i < k; i++) {
                for (uint32_t j = 0; j < dimension_partition; j++) {
                    centroids[i][j] /= dim_centroid[i];
                }
            }
        }
    }
    Debug("Distance computed := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS));

    // print counts
    for (uint32_t i = 0; i < k; i++) {
        std::cout << "len(centroids["
                    << i << "]) = "
                    << dim_centroid[i] << std::endl;
    }

    for (uint32_t i = 0; i < k; i++) {
        free(centroids[i]);
    }
    free(centroids);
    free(beholds);
    free(dim_centroid);
    Debug("OK");
    return nullptr;
}