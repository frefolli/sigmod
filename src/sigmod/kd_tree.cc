#include <sigmod/kd_tree.hh>
#include <sigmod/scoreboard.hh>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>
#include <sigmod/random.hh>

void FreeKDNode(KDNode* node) {
    if (node == nullptr)
        return;
    if (node->left != nullptr) {
        FreeKDNode(node->left);
        node->left = nullptr;
    }
    if (node->right != nullptr) {
        FreeKDNode(node->right);
        node->right = nullptr;
    }
    free(node);
}

void FreeKDTree(KDTree& tree) {
    if (tree.root != nullptr) {
        FreeKDNode(tree.root);
        tree.root = nullptr;
        tree.indexes = nullptr;
    }
}

void FreeKDForest(KDForest& forest) {
}

bool IsLeaf(const KDNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

// for interval [start, end]
// note that end is included
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim) {
    KDNode* node = (KDNode*) malloc(sizeof(KDNode));
    if (node == nullptr)
        throw std::runtime_error("insufficient memory");

    std::sort(indexes + start, indexes + end + 1, [&database, &dim](uint32_t a, uint32_t b) {
        return database.records[a].fields[dim] < database.records[b].fields[dim];
    });

    const uint32_t median = (start + end)/2;
    node->value = database.records[indexes[median]].fields[dim];
    node->index = median;
    node->dim = dim;

    // const uint32_t next_dim = RandomUINT32T(0, vector_num_dimension);
    const uint32_t next_dim = (dim + 1) % vector_num_dimension;
    
    if (median-1 != end && start <= median-1 && median-1 <= database.length) {
        node->left = BuildKDNode(database, indexes, start, median-1, next_dim);
    } else {
        node->left = nullptr;
    }
    if (median + 1 != start && median + 1 <= end && end <= database.length) {
        node->right = BuildKDNode(database, indexes, median + 1, end, next_dim);
    } else {
        node->right = nullptr;
    }
    return node;
}

KDTree BuildKDTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, uint32_t first_dim) {
    // const uint32_t next_dim = RandomUINT32T(0, vector_num_dimension);
    const uint32_t next_dim = first_dim;
    return {
        .root = BuildKDNode(database, indexes, start, end, next_dim),
        .indexes = indexes
    };
}

KDForest BuildKDForest(const Database& database, const c_map_t& C_map) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, KDTree> trees;
    for (auto cat : C_map) {
        trees[cat.first] = BuildKDTree(database, indexes, cat.second.first, cat.second.second);
    }

    return {
        .indexes = indexes,
        .trees = trees
    };
}

void SearchKDNode(const Database& database, const Query& query,
                  Scoreboard& scoreboard, const KDTree& tree,
                  const KDNode* node) {
    uint32_t index = tree.indexes[node->index];
    score_t score = distance(query, database.records[index]);
    float32_t delta = query.fields[node->dim] - node->value;

    if (scoreboard.full()) {
        if (score < scoreboard.top().score) {
            scoreboard.pop();
            scoreboard.add(index, score);
        }
    } else {
        scoreboard.add(index, score);
    }
    
    if (!IsLeaf(node)) {
        if (delta > 0) {
            if (node->right != nullptr)
                SearchKDNode(database, query, scoreboard, tree, node->right);
            if (node->left != nullptr && delta * delta < scoreboard.top().score)
                SearchKDNode(database, query, scoreboard, tree, node->left);
        } else {
            if (node->left != nullptr)
                SearchKDNode(database, query, scoreboard, tree, node->left);
            if (node->right != nullptr && delta * delta < scoreboard.top().score)
                SearchKDNode(database, query, scoreboard, tree, node->right);
        }
    }
}

void SearchKDTree(const Database& database, const Query& query, Scoreboard& scoreboard, const KDTree& tree) {
    SearchKDNode(database, query, scoreboard, tree, tree.root);
}

void SearchKDForest(const KDForest& forest, const Database& database, const c_map_t& C_map, const Query& query) {
    Scoreboard gboard;

    // std::cout << query << std::endl;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    if (query_type == BY_C || query_type == BY_C_AND_T) {
        SearchKDTree(database, query, gboard, forest.trees.at(query.v));
    } else {
        for (auto tree : forest.trees) {
            SearchKDTree(database, query, gboard, tree.second);
        }
    }

    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        // std::cout << rank << " | " << gboard.top().index << " | " << gboard.top().score << std::endl;
        gboard.pop();
        rank--;
    }
}
