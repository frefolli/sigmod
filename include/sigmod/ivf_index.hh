#ifndef IVF_INDEX_HH
#define IVF_INDEX_HH

#include <sigmod/quantization.hh>

struct IVF{

    CodeBook codebook_coarse;
    CodeBook codebook_pq;
};

IVF* MallocIVF() {

}

void coarse_quantization(IVF& invertedfile, const Database& db, const uint32_t K_coarse, const uint32_t iteration) {
    Kmeans(invertedfile.codebook_coarse, db, iteration, 0, actual_vector_size - 1, db.length);
}

#endif