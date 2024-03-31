#ifndef KD_TREE_HH
#define KD_TREE_HH

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>

enum kdnode_t {
    MIDDLE = 0,
    LEAF = 1
};

struct KDNode {
    kdnode_t type;
    float32_t median;
    uint32_t index;
    KDNode* left;
    KDNode* right;
};

struct KDTree {
    KDNode* root;
    uint32_t* indexes;
};

void FreeKDTree(KDTree& tree);
void FreeKDNode(KDNode* node);
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim);
KDTree BuildKDTree(const Database& database);
void KDTreeSearch(Result& result, const KDTree& tree, const Database& database, const Query& query);
void KDTreeSearch2(Result& result, const KDTree& tree, const Database& database, const Query& query);

#endif
