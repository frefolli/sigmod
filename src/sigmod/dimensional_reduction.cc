#include<sigmod/dimensional_reduction.hh>
#include<cmath>
#include<sigmod/config.hh>

StatisticalIndeces* MallocStatisticalIndeces(Database& database, 
    const float32_t mean, const float32_t var, const float32_t std) {
    StatisticalIndeces* si = (StatisticalIndeces*) std::malloc(sizeof(StatisticalIndeces));
    si->db = &database;
    si->mean = (float32_t*) std::malloc(sizeof(float32_t) * vector_num_dimension);
    si->var = (float32_t*) std::malloc(sizeof(float32_t) * vector_num_dimension);
    si->covarianceMatrix = (float32_t*) std::malloc(sizeof(float32_t) * vector_num_dimension + 1);
}

void InitializeStatisticalIndeces(StatisticalIndeces& si) {
    ComputeMean(si); 
    ComputeVariance(si);
}

void Standardize(StatisticalIndeces& si) {
    InitializeStatisticalIndeces(si);
    for (uint32_t i = 0; i < si.db->length; i++) {
        for (uint32_t j = 0; j < vector_num_dimension; j++) {
            si.db->records[i].fields[j]  = (si.db->records[i].fields[j] - si.mean[j]) / sqrt(si.var[j]);
        }
    }
}

void ComputeMean(StatisticalIndeces& si) {
    uint64_t n_elements = si.db->length * vector_num_dimension;
    float32_t partial_sum_per_attr = 0; 
        
    for (uint32_t j = 0; j < vector_num_dimension; j++) {
        partial_sum_per_attr = 0; 
        for (uint32_t i = 0; i < si.db->length; i++) {
            partial_sum_per_attr += si.db->records[i].fields[j];
        }
        si.mean[j] = partial_sum_per_attr / si.db->length;
    }
}

void ComputeVariance(StatisticalIndeces& si) {
    uint64_t n_elements = si.db->length * vector_num_dimension;
    float32_t partial_dev_per_attr = 0; 
        
    for (uint32_t j = 0; j < vector_num_dimension; j++) {
        partial_dev_per_attr = 0; 
        for (uint32_t i = 0; i < si.db->length; i++) {
            partial_dev_per_attr += pow(si.db->records[i].fields[j] - si.mean[j], 2);
        }
        si.var[j] = partial_dev_per_attr / si.db->length;
    }
}

const float32_t ComputeVectorNorm2(const float32_t vector[]){
    float32_t sum = 0;
    for (int32_t i = 0; i < vector_num_dimension; i++) {
        sum = vector[i] * vector[i];
    }
    return sqrt(sum);
}  

void NormalizeVector(float32_t vector[]){
    float32_t norm2 = ComputeVectorNorm2(vector);
    for (int32_t i = 0; i < vector_num_dimension; i++) {
        vector[i] /= norm2;
    }
}
float32_t* GenerateVector(const uint32_t dimension, const float32_t initial_value){
    float32_t* vector = (float32_t*) std::malloc(sizeof(float32_t) * dimension);
    for (int32_t i = 0; i < dimension; i++) {
        vector[i] = initial_value;
    }
    return vector;
}    

void pca(StatisticalIndeces& si, uint32_t iterations) {
    Standardize(si);
    float32_t r[vector_num_dimension];
    NormalizeVector(r);
    float32_t* s;
    for(uint32_t i = 0; i < iterations; i++){
        s = GenerateVector(vector_num_dimension, 0);
        for (uint32_t i = 0; i < si.db->length; i++) {
            s = s + (x * r) * x;
        }
    }

    // ! ON WORKING
}

void FreeStatisticalIndeces(StatisticalIndeces& si) {
    if (si.db != nullptr) {
        FreeDatabase(*(si.db));
    }

    if (si.mean != nullptr) {
        std::free(si.mean);
    }
    
    if (si.var != nullptr) {
        std::free(si.var);
    }

    if (si.covarianceMatrix != nullptr) {
        std::free(si.covarianceMatrix);
    }
}
