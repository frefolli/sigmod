#ifndef QUANTIZATION_HH
#define QUANTIZATION_HH

#include <sigmod/database.hh>
#include <vector>
#include <sigmod/debug.hh>
#include <cmath>

struct CodeBook{
    /* index[id_vector, ids_centroids_associated_foreach_partition] */
    std::map<uint32_t, uint8_t[M]> vector_centroid;
    /* index[id_partition, id_centroid, centroid_component] */
    std::map<uint8_t, std::map<uint8_t, float32_t[M]>> centroids;
};

void Kmeans(
    CodeBook& cb,
    const Database& database, 
    const uint32_t ITERATIONS, 
    const uint32_t start_partition_id, 
    const uint32_t end_partition_id);

inline void compute_distributions(std::vector<uint32_t>& dim_centroids) {
    score_t sum = 0;
    //uint32_t k = dim_centroids.size();

    for (uint32_t i = 0; i < K; i++) {
        sum += dim_centroids[i];
    }
    
    float32_t mean = sum/K;

    Debug("# Vectors distribuitions between centroids");
    Debug("tot := " + std::to_string(sum));
    Debug("mean := " + std::to_string(mean));
    Debug("median := " + std::to_string(dim_centroids[K/2-1]));

    sum = 0;
    for (uint32_t i = 0; i < K; i++) {
        sum += pow(dim_centroids[i] -  mean, 2);
    }
    float32_t var = ((float32_t) sum)/K;

    Debug("var := " + std::to_string(var));
    Debug("std := " + std::to_string(sqrt(var)));
}

inline score_t MSE_distorsion(Database& dataset, std::vector<std::vector<float32_t>>* CodeBook) {

}

#endif