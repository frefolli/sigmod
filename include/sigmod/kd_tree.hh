#ifndef KD_TREE_HH
#define KD_TREE_HH

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>

struct KDNode {
    float32_t value;
    uint32_t index;
    KDNode* left;
    KDNode* right;
    uint32_t dim;
};

struct KDTree {
    KDNode* root;
    uint32_t* indexes;
};

struct KDForest {
    uint32_t length;
    KDTree* trees;
};

void FreeKDForest(KDForest& forest);
void FreeKDTree(KDTree& tree);
void FreeKDNode(KDNode* node);
bool IsLeaf(const KDNode* node);
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim);
KDTree BuildKDTree(const Database& database, uint32_t first_dim = 0);
KDForest BuildKDForest(const Database& database, uint32_t length);
void KDTreeSearch(Result& result, const KDTree& tree, const Database& database, const Query& query);
void KDTreeSearch2(Result& result, const KDTree& tree, const Database& database, const Query& query);
void KDTreeSearch3(Result& result, const KDForest& forest, const Database& database, const Query& query);

#endif
