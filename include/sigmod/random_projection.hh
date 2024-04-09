#ifndef RANDOM_PROJECTION_HH
#define RANDOM_PROJECTION_HH

// Theory https://en.wikipedia.org/wiki/Random_projection

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query_set.hh>
#include <sigmod/lin_alg.hh>

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

const float32_t** RandomProjection(
    float32_t** dataset_matrix, 
    const uint32_t n_observation, 
    const uint32_t dimension, 
    const uint32_t final_dimension);

inline void MultiplyDatabaseMatrix(
    Database& dataset,
    const uint32_t dimension,
    const float32_t** prj_matrix,
    const uint32_t final_dimension) {
    float32_t* temp_vect = MallocVector(final_dimension); 
    for (uint32_t i = 0; i < dataset.length; i++) {
        for (uint32_t j = 0; j < final_dimension; j++) {
            temp_vect[j] = 0;
            for (uint32_t k = 0; k < dimension; k++) {
                temp_vect[j] += dataset.records[i].fields[k] * prj_matrix[k][j];
            }
        }
        CopyVectorFrom(temp_vect, dataset.records[i].fields, final_dimension);
    }
    FreeVector(temp_vect);
}

inline void MultiplyQuerySetMatrix(
    QuerySet& queryset,
    const uint32_t dimension,
    const float32_t** prj_matrix,
    const uint32_t final_dimension) {
    float32_t* temp_vect = MallocVector(final_dimension); 
    for (uint32_t i = 0; i < queryset.length; i++) {
        for (uint32_t j = 0; j < final_dimension; j++) {
            temp_vect[j] = 0;
            for (uint32_t k = 0; k < dimension; k++) {
                temp_vect[j] += queryset.queries[i].fields[k] * prj_matrix[k][j];
            }
        }
        CopyVectorFrom(temp_vect, queryset.queries[i].fields, final_dimension);
    }
    FreeVector(temp_vect);
}

const float32_t** RandomProjectionOnDataset(
    Database& dataset, 
    const uint32_t dimension, 
    const uint32_t final_dimension);

void RandomProjectionOnQuerySet(
    QuerySet& queryset, 
    const uint32_t dimension, 
    const uint32_t final_dimension);

void RandomProjectionGivenProjMatrix(
    float32_t** dataset_matrix, 
    const uint32_t n_observation, 
    const uint32_t dimension, 
    const float32_t** prj_matrix, 
    const uint32_t final_dimension);
    
void FreeProjectionMatrix(float32_t** matrix);

#endif