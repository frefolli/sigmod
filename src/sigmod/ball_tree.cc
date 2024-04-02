#include <sigmod/ball_tree.hh>
#include <iostream>
#include <algorithm>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>

void FreeBallNode(BallNode* node) {
    if (node == nullptr)
        return;
    if (node->left != nullptr) {
        FreeBallNode(node->left);
        node->left = nullptr;
    }
    if (node->right != nullptr) {
        FreeBallNode(node->right);
        node->right = nullptr;
    }
    free(node);
}

void FreeBallTree(BallTree& tree) {
    if (tree.root != nullptr) {
        FreeBallNode(tree.root);
        tree.root = nullptr;
        tree.indexes = nullptr;
        tree.start = 0;
        tree.end = 0;
    }
}

void FreeBallForest(BallForest& forest) {
    if (forest.trees != nullptr) {
        for (uint32_t i = 0; i < forest.length; i++) {
            FreeBallTree(forest.trees[i]);
        }
        free(forest.trees);
        forest.trees = nullptr;
        forest.length = 0;
    }
    if (forest.indexes != nullptr) {
        free(forest.indexes);
        forest.indexes = nullptr;
    }
}

bool IsLeaf(const BallNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

bool IsMiddle(const BallNode* node) {
    return (node->left != nullptr && node->right != nullptr);
}

struct Mixin {
    float32_t mins[vector_num_dimension];
    float32_t maxs[vector_num_dimension];
};

// end is not included
uint32_t DetermineMaxSpread(const Database& database, const uint32_t* indexes, Mixin& mixin, const uint32_t start, const uint32_t end) {
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        mixin.mins[i] = database.records[indexes[start]].fields[i];
        mixin.maxs[i] = database.records[indexes[start]].fields[i];
    }

    for (uint32_t j = start + 1; j < end; j++) {
        for (uint32_t i = 0; i < vector_num_dimension; i++) {
            const float32_t val = database.records[indexes[j]].fields[i];
            if (val < mixin.mins[i])
                mixin.mins[i] = val;
            if (val > mixin.maxs[i])
                mixin.maxs[i] = val;
        }
    }

    uint32_t max_spread_index = 0;
    float32_t max_spread = mixin.maxs[0] - mixin.mins[0];
    for (uint32_t i = 1; i < vector_num_dimension; i++) {
        float32_t spread = mixin.maxs[i] - mixin.mins[i];
        if (spread > max_spread) {
            max_spread = spread;
            max_spread_index = i;
        }
    }

    return max_spread_index;
}

BallNode* BuildBallNode(const Database& database, uint32_t* indexes, Mixin& mixin, const uint32_t start, const uint32_t end) {
    return nullptr;
}

BallTree BuildBallTree(const Database& database, uint32_t* indexes, Mixin& mixin, const uint32_t start, const uint32_t end) {
    if (start >= end || end > database.length)
        return {
            .root = nullptr,
            .indexes = nullptr,
            .start = 0,
            .end = 0
        };

    return {
        .root = BuildBallNode(database, indexes, mixin, start, end),
        .indexes = indexes,
        .start = start,
        .end = end
    };
}

/* It is highly probable that such category pairing (trees[i] -> C_map[i]) isn't right,
 * so in the future, in order to use it proficiently, we'll use a map {Cat -> Tree} or {Cat -> Index of Tree}.
 * */
BallForest BuildBallForest(const Database& database, const c_map_t& C_map) {
    // i try a one-dimensional axis for each division
    Mixin mixin;

    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    const uint32_t length = C_map.size();
    BallTree* trees = (BallTree*) malloc (sizeof(BallTree) * length);

    uint32_t i = 0;
    for (auto cat : C_map) {
        trees[i] = BuildBallTree(database, indexes, mixin, cat.second.first, cat.second.second + 1);
        i++;
    }

    return {
        .length = length,
        .indexes = indexes,
        .trees = trees
    };
}
