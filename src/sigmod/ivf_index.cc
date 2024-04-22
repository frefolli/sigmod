#include <sigmod/ivf_index.hh>
#include <sigmod/memory.hh>
#include <sigmod/lin_alg.hh>
#include <queue>
#include <vector>

IVF& MallocIVF(const uint32_t K_coarse_quantization, const uint32_t K_product_quantization, const uint32_t M_product_quantization, const uint32_t db_length) {
    IVF* ivf = smalloc<IVF>();
    ivf->codebook_coarse = MallocCodeBook(db_length, K_coarse_quantization, 1);
    ivf->codebook_pq = MallocCodeBook(db_length, K_product_quantization, M_product_quantization);
    ivf->residuals = smalloc<float32_t*>(db_length);
    for (uint32_t i = 0; i < db_length; i++) {
        ivf->residuals[i] = smalloc<float32_t>(actual_vector_size);
    }
    ivf->inverted_lists = MallocIVL(ivf->codebook_coarse.K);
    return *ivf;
}

NodeIVL* MallocNodeIVL(const uint16_t M_product_quantization){
    NodeIVL* nivl = smalloc<NodeIVL>();
    nivl->pq_centroids_idx = smalloc<uint16_t>(M_product_quantization);
    return nivl;
}

IVL* MallocIVL(const uint16_t dim) {
    IVL* ivl = smalloc<IVL>(dim);
    return ivl;
}

void FreeIVF(IVF* ivf) {
    if (ivf != nullptr)
    {
        if (ivf->residuals != nullptr) {
            for (uint32_t i = 0; i < ivf->codebook_coarse.db_length; i++) {
                free(ivf->residuals[i]);
            }
            free(ivf->residuals);
        }
        if(ivf->inverted_lists != nullptr) {
            for (uint32_t i = 0; i < ivf->codebook_coarse.K; i++) {
                FreeIVL(ivf->inverted_lists + i);
            }
            free(ivf->inverted_lists);
        }

        FreeCodeBook(&ivf->codebook_coarse);
        FreeCodeBook(&ivf->codebook_pq);
        
    }
}

void FreeNodeIVL(NodeIVL* node) {
    if (node != nullptr) {
        if (node->pq_centroids_idx != nullptr) {
            free(node->pq_centroids_idx);
        }
    }
}

void FreeIVL(IVL* ivl) {
    if (ivl!=nullptr) {
        for (uint32_t i = 0; i < ivl->res.size(); i++)  {
            FreeNodeIVL(ivl->res[i]);
        }
    }
}

void initializeIVF(IVF& invertedfile, const Database& db, const uint32_t iteration) {
    quantization(invertedfile.codebook_coarse, db, iteration);
    LogTime("Built Coarse quantization");

    compute_residual_vectors_for_db(invertedfile, db);
    LogTime("Built residuals");

    Database db_residuals = CreateDatabaseFromMatrix((const float32_t**)invertedfile.residuals, db.length, invertedfile.codebook_coarse.dim_partition);
    quantization(invertedfile.codebook_pq, db_residuals, iteration);
    LogTime("Built product quantization");

    for (uint32_t i = 0; i < db.length; i++) {
        uint16_t ivl_idx = invertedfile.codebook_coarse.index_vector_to_index_centroid[i][0];
        NodeIVL* n = MallocNodeIVL(invertedfile.codebook_pq.M);
        n->vector_idx = i;
        for(uint16_t j=0; j < invertedfile.codebook_pq.M; j++){
            n->pq_centroids_idx[j] = invertedfile.codebook_pq.index_vector_to_index_centroid[i][j];
        }
        invertedfile.inverted_lists[ivl_idx].res.push_back(n);
    }
    DebugIVF(invertedfile);
    
}

void compute_residual_vectors_for_db(IVF& invertedfile, const Database& db) {
    for(uint32_t i = 0; i < db.length; i++){
        DiffVectors(
            db.records[i].fields, 
            invertedfile.codebook_coarse.codewords[0].centroids[
                    invertedfile.codebook_coarse.index_vector_to_index_centroid[i][0]
                ].data,
            invertedfile.residuals[i],
            invertedfile.codebook_coarse.dim_partition);
    }
}

void searchIVF(const IVF& invertedfile, Result& result, const Query& query) {
    std::priority_queue<std::pair<score_t, uint16_t>, std::vector<std::pair<score_t, uint16_t>>, std::greater<std::pair<score_t, uint16_t>>> minHeapDistances;
    Scoreboard gboard;

    for(uint16_t i; i < invertedfile.codebook_coarse.K; i++){
        score_t dist = distance(invertedfile.codebook_coarse.codewords[0].centroids[i].data, query.fields, 0, actual_vector_size - 1);
        std::pair<score_t, uint16_t> el_heap(dist, i);
        minHeapDistances.push(el_heap);
    }
    
    uint16_t count = 0;
    for(uint16_t i; count < 100 && !minHeapDistances.empty(); i++){
        std::pair<score_t, uint16_t> course_centroid = minHeapDistances.top();
        count += invertedfile.inverted_lists[course_centroid.second].res.size();
        minHeapDistances.pop();

        float32_t* res_query = smalloc<float32_t>(actual_vector_size);
        DiffVectors(query.fields, invertedfile.codebook_coarse.codewords->centroids[course_centroid.second].data, res_query, actual_vector_size);

        for (NodeIVL* node : invertedfile.inverted_lists[course_centroid.second].res) {
            score_t dist = 0;
            for (uint32_t i = 0; i < invertedfile.codebook_pq.M; i++) {
                const uint16_t start_index = i * invertedfile.codebook_pq.M;
                const uint16_t end_index = start_index + invertedfile.codebook_pq.M - 1;
                dist += distance(invertedfile.codebook_pq.codewords[i].centroids[node->pq_centroids_idx[i]].data, res_query, start_index, end_index);
            }
            gboard.push(node->vector_idx, dist);
        }
    
        free(res_query);
    }

    
    assert(gboard.full());

    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        result.data[rank] = gboard.top().index;
        gboard.pop();
        rank--;
    }
}

void DebugIVF(const IVF& invertedfile){
    Debug("DEBUG ivf");
    for (uint32_t i = 0; i < invertedfile.codebook_coarse.K; i++){
        Debug("dimension ivl "+ std::to_string(i)+ "-esima := " +std::to_string(invertedfile.inverted_lists[i].res.size()));
    }
    
}
