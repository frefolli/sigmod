#include <sigmod/vp_tree.hh>
#include <sigmod/flags.hh>
#include <sigmod/tweaks.hh>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/random.hh>
#include <algorithm>
#include <cassert>

void FreeVPNode(VPNode* node) {
    if (node == nullptr)
        return;
    if (node->left != nullptr) {
        FreeVPNode(node->left);
        node->left = nullptr;
    }
    if (node->right != nullptr) {
        FreeVPNode(node->right);
        node->right = nullptr;
    }
    free(node);
}

void FreeVPTree(VPTree& tree) {
    if (tree.root != nullptr) {
        FreeVPNode(tree.root);
        tree.root = nullptr;
        tree.indexes = nullptr;
    }
}

void FreeVPForest(VPForest& forest) {
    for (auto tree : forest.trees)
        FreeVPTree(tree.second);
    if (forest.indexes != nullptr) {
        free(forest.indexes);
        forest.indexes = nullptr;
    }
}

bool IsLeaf(const VPNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

VPNode* BuildVPNode(const Database& database, uint32_t* indexes, score_t* distances, const uint32_t start, const uint32_t end) {
    assert(start < end && end <= database.length);

    if (end - start <= VP_NODE_SIZE) {
        VPNode* node = (VPNode*) malloc (sizeof(VPNode));
        node->start = start;
        node->end = end;
        node->center = -1;
        node->radius = -1;
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    // std::swap(indexes[RandomUINT32T(start, end)], indexes[start]);
    const uint32_t center = start;
    for (uint32_t i = start + 1; i < end; i++) {
        distances[i] = distance(database.at(indexes[center]), database.at(indexes[i]));
    }

    std::sort(indexes + start + 1, indexes + end, [&distances](const uint32_t a, const uint32_t b) {
        return distances[a] < distances[b];
    });

    const uint32_t middle = (start + end) / 2;
    const uint32_t next_start = middle;
    const uint32_t next_end = middle;

    VPNode* node = (VPNode*) malloc (sizeof(VPNode));
    node->start = start;
    node->end = end;
    node->center = center;
    node->radius = distances[middle];

    node->left = BuildVPNode(database, indexes, distances, start, next_end);
    node->right = BuildVPNode(database, indexes, distances, next_start, end);

    assert(node->left != nullptr && node->right != nullptr);
    return node;
}

VPTree BuildVPTree(const Database& database, uint32_t* indexes, score_t* distances, const uint32_t start, const uint32_t end) {
    if (start >= end || end > database.length)
        return {
            .root = nullptr,
            .indexes = nullptr
        };

    return {
        .root = BuildVPNode(database, indexes, distances, start, end),
        .indexes = indexes
    };
}

VPForest BuildVPForest(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    score_t* distances = (score_t*) malloc (sizeof(score_t) * database.length);

    std::map<uint32_t, VPTree> trees;
    for (auto cat : database.C_map) {
        trees[cat.first] = BuildVPTree(database, indexes, distances, cat.second.first, cat.second.second + 1);
    }

    free(distances);

    return {
        .indexes = indexes,
        .trees = trees
    };
}

void SearchVPNode(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const VPTree& tree,
                    const VPNode* node) {
    if (IsLeaf(node)) {
        for (uint32_t i = node->start; i < node->end; i++) {
            const uint32_t p = tree.indexes[i];
            const score_t distance_query_p = distance(query, database.at(p));
            scoreboard.push(p, distance_query_p);
        }
    } else {
        const uint32_t center = tree.indexes[node->center];
        const score_t distance_query_center = distance(query, database.at(center));
        scoreboard.push(center, distance_query_center);

        if (distance_query_center < node->radius) {
            if (scoreboard.empty() || distance_query_center - (node->radius * VP_RADIUS_AMPLIFICATION) <= scoreboard.top().score)
                SearchVPNode(database, query, scoreboard, tree, node->left);
            if (scoreboard.empty() || (node->radius * VP_RADIUS_AMPLIFICATION) - distance_query_center <= scoreboard.top().score)
                SearchVPNode(database, query, scoreboard, tree, node->right);
        } else {
            if (scoreboard.empty() || (node->radius * VP_RADIUS_AMPLIFICATION) - distance_query_center <= scoreboard.top().score)
                SearchVPNode(database, query, scoreboard, tree, node->right);
            if (scoreboard.empty() || distance_query_center - (node->radius * VP_RADIUS_AMPLIFICATION) <= scoreboard.top().score)
                SearchVPNode(database, query, scoreboard, tree, node->left);
        }
    }
}

void SearchVPTree(const Database& database, const Query& query, Scoreboard& scoreboard, const VPTree& tree) {
    SearchVPNode(database, query, scoreboard, tree, tree.root);
}

void SearchVPForest(const VPForest& forest, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    if (query_type == BY_C || query_type == BY_C_AND_T) {
        SearchVPTree(database, query, gboard, forest.trees.at(query.v));
    } else {
        for (auto tree : forest.trees) {
            SearchVPTree(database, query, gboard, tree.second);
        }
    }

    assert (gboard.full());
    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        result.data[rank] = database.indexes[gboard.top().index];
        gboard.pop();
        rank--;
    }
}
