#ifndef BALL_TREE_HH
#define BALL_TREE_HH

#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>

struct BallNode {
    uint32_t start;
    uint32_t end;
    float32_t radius;
    Record center;
    BallNode* left;
    BallNode* right;
};

struct BallTree {
    BallNode* root;
    uint32_t* indexes;
};

struct BallForest {
    uint32_t* indexes;
    std::map<uint32_t, BallTree> trees;
};

void FreeBallNode(BallNode* node);
void FreeBallTree(BallTree& tree);
void FreeBallForest(BallForest& forest);

BallNode* BuildBallNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);
BallTree BuildBallTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);
BallForest BuildBallForest(const Database& database, const c_map_t& C_map);

void SearchBallNode(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, const score_t distance_query_center);
void SearchBallTree(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree);
void SearchBallForest(const BallForest& forest, const Database& database, const c_map_t& C_map, const Query& query);

#endif
