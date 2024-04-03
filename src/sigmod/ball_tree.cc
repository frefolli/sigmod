#include <sigmod/ball_tree.hh>
#include <iostream>
#include <algorithm>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>
#include <cassert>

#define EPSILON 0.05

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
    }
}

void FreeBallForest(BallForest& forest) {
    for (auto tree : forest.trees)
        FreeBallTree(tree.second);
    if (forest.indexes != nullptr) {
        free(forest.indexes);
        forest.indexes = nullptr;
    }
}

bool IsLeaf(const BallNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

uint32_t FindFurthestPoint(const Database& database, uint32_t* indexes,
                           const uint32_t start, const uint32_t end,
                           const uint32_t target) {
    uint32_t furthest = start;
    score_t furthest_score = distance(database.records[indexes[target]], database.records[indexes[furthest]]);
    for (uint32_t i = start + 1; i < end; i++) {
        score_t score = distance(database.records[indexes[target]], database.records[indexes[i]]);
        if (score > furthest_score) {
            furthest_score = score;
            furthest = i;
        }
    }
    return furthest;
}

inline bool is_leftist(const Database& database, const uint32_t* indexes, const uint32_t a, const uint32_t b, const uint32_t x) {
    score_t da = distance(database.records[indexes[a]], database.records[indexes[x]]);
    score_t db = distance(database.records[indexes[b]], database.records[indexes[x]]);
    return (da - db) < 0;
}

BallNode* BuildBallNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
    assert(start < end && end <= database.length);

    if (end - start <= k_nearest_neighbors) {
        BallNode* node = (BallNode*) malloc (sizeof(BallNode));
        node->start = start;
        node->end = end;
        uint32_t length = end - start;
        for (uint32_t i = 0; i < vector_num_dimension; i++) {
            node->center.fields[i] = database.records[indexes[start]].fields[i] / length;
        }
        for (uint32_t j = start + 1; j < end; j++) {
            for (uint32_t i = 0; i < vector_num_dimension; i++) {
                node->center.fields[i] += database.records[indexes[j]].fields[i] / length;
            }
        }
        node->radius = 0;
        for (uint32_t j = start; j < end; j++) {
            score_t radius = distance(node->center, database.records[indexes[j]]);
            if (radius > node->radius)
                node->radius = radius;
        }
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    uint32_t a = FindFurthestPoint(database, indexes, start, end, start);
    uint32_t b = FindFurthestPoint(database, indexes, start, end, a);
    
    if (a != start)
        std::swap(indexes[a], indexes[start]);
    if (b != end - 1)
        std::swap(indexes[b], indexes[end - 1]);

    uint32_t i = start + 1;
    uint32_t j = end - 2;

    while (i < j) {
        while(i <= j && is_leftist(database, indexes, a, b, i)) {
            i++;
        }
        while(i <= j && !is_leftist(database, indexes, a, b, j)) {
            j--;
        }
        if (i < j) {
            std::swap(indexes[i], indexes[j]);
            i++; j--;
        }
    }

    uint32_t next_start = j+1;
    uint32_t next_end = i;

    BallNode* node = (BallNode*) malloc (sizeof(BallNode));
    node->start = start;
    node->end = end;

    node->left = BuildBallNode(database, indexes, start, next_end);
    node->right = BuildBallNode(database, indexes, next_start, end);

    assert(node->left != nullptr && node->right != nullptr);
    
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        node->center.fields[i] = (node->left->center.fields[i] + node->right->center.fields[i]) / 2;
    }

    node->radius = (node->left->radius + node->right->radius + distance(node->left->center, node->right->center)) / 2;
    // node->radius = node->left->radius + node->right->radius - (distance(node->left->center, node->right->center)) / 2;

    return node;
}

BallTree BuildBallTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
    if (start >= end || end > database.length)
        return {
            .root = nullptr,
            .indexes = nullptr
        };

    return {
        .root = BuildBallNode(database, indexes, start, end),
        .indexes = indexes
    };
}

BallForest BuildBallForest(const Database& database, const c_map_t& C_map) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, BallTree> trees;
    for (auto cat : C_map) {
        trees[cat.first] = BuildBallTree(database, indexes, cat.second.first, cat.second.second + 1);
    }

    return {
        .indexes = indexes,
        .trees = trees
    };
}

void SearchBallNode(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, const score_t distance_query_center) {
    if (IsLeaf(node)) {
        for (uint32_t i = node->start; i < node->end; i++) {
            const uint32_t p = tree.indexes[i];
            const score_t distance_query_p = distance(query, database.records[p]);
            if (scoreboard.full()) {
                if (distance_query_p < scoreboard.top().score) {
                    scoreboard.pop();
                    scoreboard.add(p, distance_query_p);
                }
            } else {
                scoreboard.add(p, distance_query_p);
            }
        }
    } else {
        const score_t distance_query_left = distance(query, node->left->center);
        const score_t distance_query_right = distance(query, node->right->center);

        if (distance_query_left < distance_query_right) {
            if (scoreboard.empty() || distance_query_left - (node->left->radius * EPSILON) < scoreboard.top().score)
                SearchBallNode(database, query, scoreboard, tree, node->left, distance_query_left);
            if (scoreboard.empty() || distance_query_right - (node->right->radius * EPSILON) < scoreboard.top().score)
                SearchBallNode(database, query, scoreboard, tree, node->right, distance_query_right);
        } else {
            if (scoreboard.empty() || distance_query_right - (node->right->radius * EPSILON) < scoreboard.top().score)
                SearchBallNode(database, query, scoreboard, tree, node->right, distance_query_right);
            if (scoreboard.empty() || distance_query_left - (node->left->radius * EPSILON) < scoreboard.top().score)
                SearchBallNode(database, query, scoreboard, tree, node->left, distance_query_left);
        }
    }
}

void SearchBallTree(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree) {
    score_t distance_query_root = distance(query, tree.root->center);
    if (scoreboard.empty() || distance_query_root - (tree.root->radius * EPSILON) < scoreboard.top().score)
        SearchBallNode(database, query, scoreboard, tree, tree.root, distance_query_root);
}

void SearchBallForest(const BallForest& forest, const Database& database, const c_map_t& C_map, Result& result, const Query& query) {
    Scoreboard gboard;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    if (query_type == BY_C || query_type == BY_C_AND_T) {
        SearchBallTree(database, query, gboard, forest.trees.at(query.v));
    } else {
        for (auto tree : forest.trees) {
            SearchBallTree(database, query, gboard, tree.second);
        }
    }

    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        result.data[rank] = gboard.top().index;
        gboard.pop();
        rank--;
    }
}
