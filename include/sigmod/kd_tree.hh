#ifndef KD_TREE_HH
#define KD_TREE_HH

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>
#include <sigmod/scoreboard.hh>

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
    uint32_t* indexes;
    std::map<uint32_t, KDTree> trees;
};

bool IsLeaf(const KDNode* node);

void FreeKDForest(KDForest& forest);
void FreeKDTree(KDTree& tree);
void FreeKDNode(KDNode* node);

KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim);
KDTree BuildKDTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t first_dim = 0);
KDForest BuildKDForest(const Database& database, const c_map_t& C_map);

void SearchKDNode(const Database& database, const Query& query,
                  Scoreboard& scoreboard, const KDTree& tree,
                  const KDNode* node);
void SearchKDTree(const Database& database, const Query& query, Scoreboard& scoreboard, const KDTree& tree);
void SearchKDForest(const KDForest& forest, const Database& database, const c_map_t& C_map, const Query& query);

#endif
