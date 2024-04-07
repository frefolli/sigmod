#ifndef RANDOM_PROJECTION_HH
#define RANDOM_PROJECTION_HH

// Theory https://en.wikipedia.org/wiki/Random_projection

#include <sigmod/config.hh>

/**
 * Riduce dimensionality of the dataset by projecting all data from starting 
 * dimension d to a final dimension k. Size of starting matrix will be preserved,
 * but new component are injected inside the first k colummns, the others remain 
 * unchanged. Take in mind that after the reduction, you have to access to first 
 * k columns.
 */

const float32_t** GenerateProjectionMatrix(
    const uint32_t final_dimension, 
    const uint32_t initial_dimension);

const float32_t** RamdomProjection(
    float32_t** dataset_matrix, 
    const uint32_t n_observation, 
    const uint32_t dimension, 
    const uint32_t final_dimension);

void RamdomProjectionGivenProjMatrix(
    float32_t** dataset_matrix, 
    const uint32_t n_observation, 
    const uint32_t dimension, 
    const float32_t** prj_matrix, 
    const uint32_t final_dimension);

void FreeProjectionMatrix(float32_t** matrix);

#endif