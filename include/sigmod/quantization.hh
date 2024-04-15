#ifndef QUANTIZATION_HH
#define QUANTIZATION_HH

#include <sigmod/database.hh>
#include <sigmod/debug.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>
#include <sigmod/scoreboard.hh>
#include <vector>
#include <cmath>
#include <cassert>

const uint8_t M = 10; // #partitions
const uint32_t K = 256; // #clusters per partition
const uint8_t dim_partition = actual_vector_size / M;


struct CodeBook{
    /* indeces := [id_vector, ids_centroids_associated_foreach_partition] */
    std::map<uint32_t, uint8_t[M]> vector_centroid;
    /* indeces := [id_partition, id_centroid, centroid_component] */
    std::map<uint8_t, std::map<uint8_t, float32_t[M]>> centroids;
};

void Kmeans(
    CodeBook& cb,
    const Database& database, 
    const uint32_t ITERATIONS, 
    const uint32_t start_partition_id, 
    const uint32_t end_partition_id);

inline void compute_distributions(const std::vector<uint32_t>& dim_centroids) {
    score_t sum = 0;
    uint32_t k = dim_centroids.size();

    for (uint32_t i = 0; i < K; i++) {
        sum += dim_centroids[i];
    }
    
    float32_t mean = sum/k;

    Debug("# Vectors distribuitions between centroids");
    Debug("tot := " + std::to_string(sum));
    Debug("mean := " + std::to_string(mean));

    sum = 0;
    for (uint32_t i = 0; i < K; i++) {
        sum += pow(dim_centroids[i] -  mean, 2);
    }
    float32_t var = ((float32_t) sum)/K;

    Debug("var := " + std::to_string(var));
    Debug("std := " + std::to_string(sqrt(var)));
}

inline score_t distance(const float32_t* centroid, const float32_t* vector, const uint32_t start_index_partition, const uint32_t end_index_partition) {
    #ifdef TRACK_DISTANCE_COMPUTATIONS
        SIGMOD_DISTANCE_COMPUTATIONS++;
    #endif
    score_t sum = 0;
    for (uint32_t i = 0; i < end_index_partition - start_index_partition + 1; i++) {
        score_t m = centroid[i] - vector[i + start_index_partition];
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

/* 
* compute matrix of distances between query and all centroids of all partitions 
* matr_dist := score_t[M][K]
*/
void PreprocessingQuery(score_t matr_dist[M][K], const float32_t* query, const CodeBook& cb);
const score_t ADC(const score_t matr_dist[M][K], const CodeBook& cb, const uint32_t index_vector);

void SearchExaustivePQ(const CodeBook& cb, const Database& database, Result& result, const Query& query);

#endif
