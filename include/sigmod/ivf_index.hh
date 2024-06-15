#ifndef IVF_INDEX_HH
#define IVF_INDEX_HH
/** @file ivf_index.hh */

#include <sigmod/quantization.hh>
#include <sigmod/memory.hh>

//std::priority_queue<std::pair<score_t, uint16_t>, std::vector<std::pair<score_t, uint16_t>>, std::greater<std::pair<score_t, uint16_t>>>
typedef std::pair<score_t, uint16_t> distance_centroid_t;

struct NodeIVL{
    uint32_t vector_idx;
    uint16_t* pq_centroids_idx; // M_pq
};

struct IVL{
    std::vector<NodeIVL*> res; // # residuals mapped in the same coarse centroid (length)
};

struct IVF{
    CodeBook codebook_coarse;
    CodeBook codebook_pq;
    float32_t** residuals; // db_length x actual_vector_size
    IVL* inverted_lists; // K_coarse
};

IVF* MallocIVF(const uint32_t K_coarse_quantization, const uint32_t K_product_quantization, const uint32_t M_product_quantization, const uint32_t db_length);
NodeIVL* MallocNodeIVL(const uint16_t M_product_quantization);
IVL* MallocIVL(const uint16_t K_coarse);
void FreeIVF(IVF* ivf);
void FreeNodeIVL(NodeIVL* node);
void FreeIVL(IVL* ivl);

void compute_residual_vectors_for_db(IVF& invertedfile, const Database& db);
void initializeIVF(IVF& invertedfile, const Database& db, const uint32_t iteration);
void searchIVF(const IVF& invertedfile, const Database& database, Result& result, const Query& query);

void DebugIVF(const IVF& invertedfile);

#endif
