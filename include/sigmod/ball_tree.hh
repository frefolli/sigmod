#ifndef BALL_TREE_HH
#define BALL_TREE_HH

#include <sigmod/database.hh>
#include <sigmod/query.hh>

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

BallForest BuildBallForest(const Database& database, const c_map_t& C_map);
void FreeBallForest(BallForest& forest);
void PrintBallForest(const BallForest& forest);
void SearchBallForest(const BallForest& forest, const Database& database, const c_map_t& C_map, const Query& query);

#endif
