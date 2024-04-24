#ifndef VP_TREE_HH
#define VP_TREE_HH

#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/solution.hh>

struct VPNode {
    uint32_t start;
    uint32_t end;
    uint32_t center;
    float32_t radius;
    VPNode* left;
    VPNode* right;
};

struct VPTree {
    VPNode* root;
    uint32_t* indexes;
};

struct VPForest {
    uint32_t* indexes;
    std::unordered_map<uint32_t, VPTree> trees;
};

void FreeVPNode(VPNode* node);
void FreeVPTree(VPTree& tree);
void FreeVPForest(VPForest& forest);

VPNode* BuildVPNode(const Database& database, uint32_t* indexes, score_t* distances, const uint32_t start, const uint32_t end);
VPTree BuildVPTree(const Database& database, uint32_t* indexes, score_t* distances, const uint32_t start, const uint32_t end);
VPForest BuildVPForest(const Database& database);

void SearchVPNode(const Database& database, const Query& query,
                  Scoreboard& scoreboard, const VPTree& tree,
                  const VPNode* node, const score_t distance_query_center);
void SearchVPTree(const Database& database, const Query& query, Scoreboard& scoreboard, const VPTree& tree);
void SearchVPForest(const VPForest& forest, const Database& database, Result& result, const Query& query);

#endif
