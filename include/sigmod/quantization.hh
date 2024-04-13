#ifndef QUANTIZATION_HH
#define QUANTIZATION_HH

#include <sigmod/database.hh>

float32_t* Kmeans(
    const Database& database, 
    const uint32_t ITERATIONS, 
    const uint32_t start_partition_id, 
    const uint32_t end_partition_id,  
    const uint32_t k);



#endif