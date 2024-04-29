#include<sigmod/statistical_indeces.hh>
#include<cmath>
#include<sigmod/lin_alg.hh>
#include<sigmod/memory.hh>

StatisticalIndeces* MallocStatisticalIndeces(Database& database, 
    const float32_t mean, const float32_t var, const float32_t std) {
    StatisticalIndeces* si = smalloc<StatisticalIndeces>(sizeof(StatisticalIndeces));
    si->db = &database;
    si->mean = (float32_t*) smalloc<float32_t>(vector_num_dimension);
    si->var = (float32_t*) smalloc<float32_t>(vector_num_dimension);
    return si;
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

}
