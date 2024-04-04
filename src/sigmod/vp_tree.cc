#include <sigmod/vp_tree.hh>
#include <sigmod/flags.hh>
#include <sigmod/tweaks.hh>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>
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

VPNode* BuildVPNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
    assert(start < end && end <= database.length);

    if (end - start <= GAMMA) {
        VPNode* node = (VPNode*) malloc (sizeof(VPNode));
        node->start = start;
        node->end = end;
        node->center = -1;
        node->radius = -1;
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    const uint32_t center = start;
    // skip center ....
    std::sort(indexes + start + 1, indexes + end, [&database, &indexes, &center](const uint32_t a, const uint32_t b) {
        return distance(database.at(indexes[center]), database.at(a)) < distance(database.at(indexes[center]), database.at(b));
    });

    const uint32_t middle = (start + end) / 2;
    const uint32_t next_start = middle;
    const uint32_t next_end = middle;

    std::cout << start << "|" << middle << "|" << end << std::endl;

    VPNode* node = (VPNode*) malloc (sizeof(VPNode));
    node->start = start;
    node->end = end;
    node->center = center;
    node->radius = distance(database.at(indexes[center]), database.at(indexes[middle]));

    node->left = BuildVPNode(database, indexes, start, next_end);
    node->right = BuildVPNode(database, indexes, next_start, end);

    assert(node->left != nullptr && node->right != nullptr);
    return node;
}

VPTree BuildVPTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
    if (start >= end || end > database.length)
        return {
            .root = nullptr,
            .indexes = nullptr
        };

    return {
        .root = BuildVPNode(database, indexes, start, end),
        .indexes = indexes
    };
}

VPForest BuildVPForest(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, VPTree> trees;
    for (auto cat : database.C_map) {
        trees[cat.first] = BuildVPTree(database, indexes, cat.second.first, cat.second.second + 1);
    }

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
            SearchVPNode(database, query, scoreboard, tree, node->left);
            if (scoreboard.empty() || (node->radius * EPSILON) - distance_query_center < scoreboard.top().score)
                SearchVPNode(database, query, scoreboard, tree, node->right);
        } else {
            SearchVPNode(database, query, scoreboard, tree, node->right);
            if (scoreboard.empty() || distance_query_center - (node->radius * EPSILON) < scoreboard.top().score)
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
