#ifndef QUANTIZATION_HH
#define QUANTIZATION_HH

#include <sigmod/database.hh>
#include <vector>
#include <sigmod/debug.hh>
#include <cmath>

std::vector<std::vector<float32_t>> Kmeans(
    const Database& database, 
    const uint32_t ITERATIONS, 
    const uint32_t start_partition_id, 
    const uint32_t end_partition_id,  
    const uint32_t k);

inline void compute_distributions(std::vector<uint32_t>& dim_centroids) {
    score_t sum = 0;
    uint32_t k = dim_centroids.size();
    for (uint32_t i = 0; i < k; i++) {
            sum += dim_centroids[i];
    }
    
    float32_t mean =((float32_t) sum)/dim_centroids.size();

    Debug("# Vectors final distribuitions between centroids");
    Debug("tot := " + std::to_string(sum));
    Debug("mean := " + std::to_string(mean));
    Debug("median := " + std::to_string(dim_centroids[k/2-1]));

    sum = 0;
    for (uint32_t i = 0; i < k; i++) {
        sum += pow(dim_centroids[i] -  mean, 2);
    }
    Debug("var := " + std::to_string(sum/k));
    Debug("std := " + std::to_string(sqrt(sum/k)));
}


#endif