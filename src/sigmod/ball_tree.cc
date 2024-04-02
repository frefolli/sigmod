#include <sigmod/ball_tree.hh>
#include <iostream>
#include <algorithm>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>

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
    const uint32_t dim = DetermineMaxSpread(database, indexes, mixin, start, end);

    std::sort(indexes + start, indexes + end, [&database, &dim](uint32_t a, uint32_t b) {
        return database.records[a].fields[dim] < database.records[b].fields[dim];
    });

    uint32_t median_index = (start + end) / 2;
    uint32_t median_record = indexes[median_index];
    float32_t median_value = database.records[median_record].fields[dim];
    float32_t min_value = database.records[indexes[start]].fields[dim];
    float32_t max_value = database.records[indexes[end - 1]].fields[dim];
    float32_t radius = std::max((max_value - median_value), (median_value - min_value));

    BallNode* node = (BallNode*) malloc(sizeof(BallNode));

    node->dim = dim;
    node->index = median_index;
    node->record = median_record;
    node->value = median_value;
    node->radius = radius;

    if (start >= median_index || median_index > database.length) {
        node->left = nullptr;
    } else {
        node->left = BuildBallNode(database, indexes, mixin, start, median_index);
    }

    if (median_index + 1 >= end || end > database.length) {
        node->right = nullptr;
    } else {
        node->right = BuildBallNode(database, indexes, mixin, median_index + 1, end);
    }

    return node;
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

void PrintBallNode(BallNode* node) {
    if (node == nullptr)
        return;
    std::cout << "(" << node->dim;
    if (node->left != nullptr) {
        std::cout << " ";
        PrintBallNode(node->left);
    }
    if (node->right != nullptr) {
        std::cout << " ";
        PrintBallNode(node->right);
    }
    std::cout << ")";
}

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

void PrintBallForest(const BallForest& forest) {
    for (uint32_t i = 0; i < forest.length; i++) {
        std::cout << i << " => ";
        PrintBallNode(forest.trees[i].root);
        std::cout << std::endl;
    }
}

bool IsLeaf(const BallNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

bool IsMiddle(const BallNode* node) {
    return (node->left != nullptr && node->right != nullptr);
}

void SearchBallNode(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree, const BallNode* node) {
    const score_t distance_of_query_from_pivot = distance(query, database.records[node->record]);

    if (scoreboard.size() > 0) {
        const score_t distance_of_query_from_worst = scoreboard.top().score;
        if (distance_of_query_from_pivot - node->radius >= distance_of_query_from_worst)
            return;
    }

    if (scoreboard.size() == k_nearest_neighbors) {
        const score_t distance_of_query_from_worst = scoreboard.top().score;
        if (distance_of_query_from_pivot < distance_of_query_from_worst) {
            scoreboard.pop();
            scoreboard.add(node->record, distance_of_query_from_pivot);
        }
    } else {
        scoreboard.add(node->record, distance_of_query_from_pivot);
    }
    if (node->left != nullptr) {
        if (node->right == nullptr) {
            SearchBallNode(database, query, scoreboard, tree, node->left);
        } else {
            const score_t distance_of_query_from_left = distance(query, database.records[node->left->record]);
            const score_t distance_of_query_from_right = distance(query, database.records[node->right->record]);
            if (distance_of_query_from_left < distance_of_query_from_right) {
                SearchBallNode(database, query, scoreboard, tree, node->left);
                SearchBallNode(database, query, scoreboard, tree, node->right);
            } else {
                SearchBallNode(database, query, scoreboard, tree, node->right);
                SearchBallNode(database, query, scoreboard, tree, node->left);
            }
        }
    } else if (node->right != nullptr) {
        SearchBallNode(database, query, scoreboard, tree, node->right);
    }
}

inline void ExaustiveSearch(const Database& database, const Query& query, Scoreboard& scoreboard, const uint32_t start_index, const uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        PushCandidate(scoreboard, database.records[i], query, i);
        if (scoreboard.size() > k_nearest_neighbors) {
            scoreboard.pop();
        }
    }
}

void SearchBallForest(const BallForest& forest, const Database& database, const c_map_t& C_map, const Query& query) {
    // for now i assume to use the first tree of the forest, which usually is thick.

    Scoreboard global_scoreboard;
    for (uint32_t j = 0; j < forest.length; j++) {
        Scoreboard scoreboard;
        SearchBallNode(database, query, scoreboard, forest.trees[j], forest.trees[j].root);
        while(!scoreboard.empty()) {
            global_scoreboard.consider(scoreboard.top());
            scoreboard.pop();
        }
    }

    uint32_t i = global_scoreboard.size() - 1;
    while(!global_scoreboard.empty()) {
        std::cout << i << " | " << global_scoreboard.top().index << " | " << global_scoreboard.top().score << std::endl;
        global_scoreboard.pop();
        i -= 1;
    }

    ExaustiveSearch(database, query, global_scoreboard, 0, database.length);

    i = global_scoreboard.size() - 1;
    while(!global_scoreboard.empty()) {
        std::cout << i << " | " << global_scoreboard.top().index << " | " << global_scoreboard.top().score << std::endl;
        global_scoreboard.pop();
        i -= 1;
    }
}
