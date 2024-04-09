#include<sigmod/dimensional_reduction.hh>
#include<sigmod/lin_alg.hh>

ComponentResults* MallocComponentResults(const uint32_t n_principal_components, const uint32_t dimension) {
    ComponentResults* cr = (ComponentResults*) std::malloc(sizeof(ComponentResults));

    cr->eigenvalue = (float32_t*) std::malloc(sizeof(float32_t) * n_principal_components);
    cr->r = (float32_t**) std::malloc(sizeof(float32_t) * n_principal_components * dimension);

    return cr;
}

/**
 * @brief 
 * 
 * PSEUDOCODE
 *  r = a random vector of length p
 *  r = r / norm(r)
 *  do c times:
 *      s = 0 (a vector of length p)
 *      for each row x in X
 *              s = s + (x ⋅ r) x
 *      λ = rTs // λ is the eigenvalue
 *      error = |λ ⋅ r − s|
 *      r = s / norm(s)
 *      exit if error < tolerance
 *  return λ, r
 * 
 * https://en.wikipedia.org/wiki/Principal_component_analysis
 * 
 * @param si 
 * @param iterations n iterations that algorithm compute
 * @param threshold threshold 
 * @return a ComponentResults with the first most important eigenvalue and its 
 *  associated unit vector r of covariance matrix
 */

ComponentResults* pca(const StatisticalIndeces& si, const uint32_t iterations, float32_t threshold) {
    float32_t* r = MallocVector(vector_num_dimension);
    NormalizeVector(r, r, vector_num_dimension);
    float32_t* s;
    float32_t* temp_vector;
    float32_t temp = 0;
    float32_t eigenvalue = 0;

    for(uint32_t i = 0; i < iterations; i++){

        s = MallocVector(vector_num_dimension, 0);
        temp_vector = MallocVector(vector_num_dimension, 0);
        for (uint32_t i = 0; i < si.db->length; i++) {
            float32_t* x = si.db->records[i].fields;
            temp = 0;
            ScalarProduct(x, r, temp, vector_num_dimension);
            ScalarDotVector(temp, x, temp_vector, vector_num_dimension);
            SumVectors(s, temp_vector, s, vector_num_dimension);
        }

        // computing heigvalue of the first component
        ScalarProduct(r, s, eigenvalue, vector_num_dimension);

        // computing error vector
        ScalarDotVector(eigenvalue, r, temp_vector, vector_num_dimension);
        DiffVectors(temp_vector, s, temp_vector, vector_num_dimension);
        
        // updating r for the next iteration
        NormalizeVector(s, r, vector_num_dimension);

        FreeVector(temp_vector);
        FreeVector(s);

        if (abs(ComputeVectorNorm2(temp_vector, vector_num_dimension)) < threshold) {
            break;
        }
    }

    ComponentResults* cr = MallocComponentResults(1, vector_num_dimension);
    cr->eigenvalue[0] = eigenvalue;
    CopyVectorFrom(r, cr->r[0], vector_num_dimension);
    FreeVector(r);

    return cr;
}


void FreeComponentResults(ComponentResults* cr) {
    if(cr->eigenvalue != nullptr){
        std::free(cr->eigenvalue);
    }
    if(cr->r != nullptr){   
        std::free(cr->r);
    }
}