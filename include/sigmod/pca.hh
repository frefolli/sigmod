#ifndef PCA_HH
#define PCA_HH
/** @file pca.hh */

#include <sigmod/statistical_indeces.hh>

struct ComponentResults{
    float32_t* eigenvalue;
    float32_t** r;
};

ComponentResults* MallocComponentResults(const uint32_t n_principal_components, 
    const uint32_t dimension);

ComponentResults* pca(const StatisticalIndeces& si, const uint32_t iterations, float32_t threshold);

void FreeComponentResults(ComponentResults* cr);

#endif
