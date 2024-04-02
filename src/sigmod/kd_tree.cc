#include <sigmod/kd_tree.hh>
#include <sigmod/scoreboard.hh>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>
#include <sigmod/random.hh>

void FreeKDNode(KDNode* node) {
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
    }
    if (tree.indexes != nullptr) {
        free(tree.indexes);
        tree.indexes = nullptr;
    }
}

void FreeKDForest(KDForest& forest) {
    if (forest.trees != nullptr) {
        for (uint32_t i = 0; i < forest.length; i++)
            FreeKDTree(forest.trees[i]);
        free(forest.trees);
        forest.trees = nullptr;
        forest.length = 0;
    }
}

bool IsLeaf(const KDNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

// for interval [start, end]
// note that end is included
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim) {
    // std::cout << "DEBUG | " << start << " ; " << end << " ; " << dim << std::endl;
    if (start > end || end >= database.length)
        return nullptr;

    KDNode* node = (KDNode*) malloc(sizeof(KDNode));
    if (node == nullptr)
        throw std::runtime_error("insufficient memory");

    std::sort(indexes + start, indexes + end + 1, [&database, &dim](uint32_t a, uint32_t b) {
        return database.records[a].fields[dim] < database.records[b].fields[dim];
    });

    const uint32_t median = start + (end - start)/2;
    // std::cout << start << "|" << median-1 << "|" << median+1 << "|" << end << std::endl;
    node->value = database.records[indexes[median]].fields[dim];
    node->index = median;
    node->dim = dim;
    // const uint32_t next_dim = RandomUINT32T(0, vector_num_dimension);
    const uint32_t next_dim = (dim + 1) % vector_num_dimension;
    if (median - 1 != end) {
        node->left = BuildKDNode(database, indexes, start, median - 1, next_dim);
    } else {
        node->left = nullptr;
    }
    if (median + 1 != start) {
        node->right = BuildKDNode(database, indexes, median + 1, end, next_dim);
    } else {
        node->right = nullptr;
    }

    return node;
}

KDTree BuildKDTree(const Database& database, uint32_t first_dim) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }
    // const uint32_t next_dim = RandomUINT32T(0, vector_num_dimension);
    const uint32_t next_dim = first_dim;
    return {
        .root = BuildKDNode(database, indexes, 0, database.length - 1, next_dim),
        .indexes = indexes
    };
}

KDForest BuildKDForest(const Database& database, uint32_t length) {
    KDTree* trees = (KDTree*) malloc (sizeof(KDTree) * length);
    const uint32_t dim_shift = vector_num_dimension / length;
    uint32_t first_dim = RandomUINT32T(0, vector_num_dimension);
    // uint32_t first_dim = 0;
    for (uint32_t i = 0; i < length; i++) {
        trees[i] = BuildKDTree(database, first_dim);
        first_dim = RandomUINT32T(0, vector_num_dimension);
        // first_dim += dim_shift;
    }
    return {
        .length = length,
        .trees = trees
    };
}

void BottomUpKDTreeSearch(Scoreboard& scoreboard,
                          const KDTree& tree,
                          const Database& database,
                          const Query& query) {
    KDNode* current_node = tree.root;

    if (current_node == nullptr) {
        throw std::runtime_error("KDTreeSearch! tree.node == nullptr");
    }

    while(!IsLeaf(current_node)) {
        PushCandidate(scoreboard, database.records[tree.indexes[current_node->index]], query, tree.indexes[current_node->index]);
        if (query.fields[current_node->dim] > current_node->value) {
            if (current_node->right != nullptr) {
                current_node = current_node->right;
            } else {
                break;
            }
        } else {
            if (current_node->left != nullptr) {
                current_node = current_node->left;
            } else {
                break;
            }
        }
    }

    // throw std::runtime_error("Interrupt");

    const uint32_t center = current_node->index;
    PushCandidate(scoreboard, database.records[tree.indexes[center]], query, tree.indexes[center]);

    uint32_t ll = center;
    uint32_t rr = center;

    const uint32_t shifts = 2 * k_nearest_neighbors;
    uint32_t i = 0;

    while(i < shifts) {
        if (ll > 0) {
            ll--;
            PushCandidate(scoreboard, database.records[tree.indexes[ll]], query, tree.indexes[ll]);
        }
        if (rr < database.length - 1) {
            rr++;
            PushCandidate(scoreboard, database.records[tree.indexes[rr]], query, tree.indexes[rr]);
        }
        i++;
    }
}

void RecursiveKDTreeSearch(Scoreboard& scoreboard,
                           const KDTree& tree,
                           const KDNode* node,
                           const Database& database,
                           const Query& query,
                           int jumps) {
    if (node != nullptr) {
        uint32_t index = tree.indexes[node->index];
        score_t score = distance(query, database.records[index]);
        float32_t delta = query.fields[node->dim] - node->value;

        PushCandidate(scoreboard, database.records[index], query, index);
        if (!IsLeaf(node)) {
            if (delta > 0) {
                RecursiveKDTreeSearch(scoreboard, tree, node->right, database, query, jumps);
                bool search_other_branch = (delta * delta < scoreboard.top().score && jumps > 0);
                if (search_other_branch)
                    RecursiveKDTreeSearch(scoreboard, tree, node->left, database, query, jumps - 1);
            } else {
                RecursiveKDTreeSearch(scoreboard, tree, node->left, database, query, jumps);
                bool search_other_branch = (delta * delta < scoreboard.top().score && jumps > 0);
                if (search_other_branch)
                    RecursiveKDTreeSearch(scoreboard, tree, node->right, database, query, jumps - 1);
            }
        }
    }
}

void KDTreeSearch2(Result& result,
                   const KDTree& tree,
                   const Database& database,
                   const Query& query) {
    Scoreboard scoreboard;
    RecursiveKDTreeSearch(scoreboard, tree, tree.root, database, query, vector_num_dimension + 1);

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}

void KDTreeSearch(Result& result,
                  const KDTree& tree,
                  const Database& database,
                  const Query& query) {
    Scoreboard scoreboard;
    BottomUpKDTreeSearch(scoreboard, tree, database, query);

    if (scoreboard.size() != k_nearest_neighbors) {
        throw std::runtime_error("Bruh wtf!");
    }

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}

void KDTreeSearch3(Result& result,
                   const KDForest& forest,
                   const Database& database,
                   const Query& query) {
    Scoreboard scoreboard;
    for (uint32_t i = 0; i < forest.length; i++) {
        BottomUpKDTreeSearch(scoreboard, forest.trees[i], database, query);
    }

    if (scoreboard.size() != k_nearest_neighbors) {
        std::cout << scoreboard.size() << std::endl;
        throw std::runtime_error("Bruh wtf!");
    }

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}
