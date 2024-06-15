#ifndef BALL_TREE_HH
#define BALL_TREE_HH
/** @file ball_tree.hh */

#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/solution.hh>

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
    std::unordered_map<uint32_t, BallTree> trees;
};

void FreeBallNode(BallNode* node);
void FreeBallTree(BallTree& tree);
void FreeBallForest(BallForest& forest);

BallNode* BuildBallNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);
BallTree BuildBallTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);
BallForest BuildBallForest(const Database& database);

void SearchBallNode(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, score_t& Dsofar, const score_t Dminp);
void SearchBallNodeByT(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, score_t& Dsofar, const score_t Dminp);

void SearchBallTree(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree);
void SearchBallTreeByT(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree);
void SearchBallForest(const BallForest& forest, const Database& database, Result& result, const Query& query);

#endif
