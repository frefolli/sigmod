#include <sigmod/kd_tree.hh>
#include <sigmod/scoreboard.hh>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>

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

// for interval [start, end]
// note that end is included
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim) {
    // std::cout << "DEBUG | " << start << " ; " << end << " ; " << dim << std::endl;
    if (start > end || end >= database.length)
        return nullptr;

    KDNode* node = (KDNode*) malloc(sizeof(KDNode));
    if (start < end) {
        if (node == nullptr)
            throw std::runtime_error("insufficient memory");
        std::sort(indexes + start, indexes + end + 1, [&database, &dim](uint32_t a, uint32_t b) {
            return database.records[a].fields[dim] < database.records[b].fields[dim];
        });

        const uint32_t median = (start+end)/2;

        node->type = MIDDLE;
        node->median = database.records[indexes[median]].fields[dim];
        node->index = median;
        node->left = BuildKDNode(database, indexes, start, median - 1, (dim + 1) % vector_num_dimension);
        node->right = BuildKDNode(database, indexes, median + 1, end, (dim + 1) % vector_num_dimension);
    } else {
        if (node == nullptr)
            throw std::runtime_error("insufficient memory");
        node->type = LEAF;
        node->median = database.records[indexes[start]].fields[dim];
        node->index = start;
        node->left = nullptr;
        node->right = nullptr;
        // std::cout << "DEBUG | " << start << " ; " << end << " ; " << dim << std::endl;
    }
    // std::cout << "DEBUG | " << start << " ; " << end << " ; " << dim << std::endl;
    return node;
}

KDTree BuildKDTree(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }
    return {
        .root = BuildKDNode(database, indexes, 0, database.length - 1, 0),
        .indexes = indexes
    };
}

void KDTreeSearch(Result& result,
                  const KDTree& tree,
                  const Database& database,
                  const Query& query) {
    KDNode* current_node = tree.root;

    if (current_node == nullptr) {
        throw std::runtime_error("KDTreeSearch! tree.node == nullptr");
    }

    uint32_t dim = 0;
    while(current_node->type != LEAF) {
        if (query.fields[dim] > current_node->median) {
            current_node = current_node->right;
        } else {
            current_node = current_node->left;
        }
        dim = (dim + 1) % vector_num_dimension;
        if (current_node == nullptr) {
            throw std::runtime_error("KDTreeSearch! current_node == nullptr");
        }
    }

    Scoreboard scoreboard;
    uint32_t center = current_node->index;
    uint32_t index = tree.indexes[center];
    PushCandidate(scoreboard, database.records[index], query, index);

    uint32_t left_shift = (k_nearest_neighbors/2);
    uint32_t right_shift = (k_nearest_neighbors/2);

    if (database.length - center <= right_shift) {
        right_shift = database.length - center - 1;
        left_shift = k_nearest_neighbors - right_shift;
    } else if (center < left_shift) {
        left_shift = center;
        right_shift = k_nearest_neighbors - left_shift;
    }

    for (uint32_t i = 1; i <= left_shift; i++) {
        if (center < i)
            std::cout << "L whops: " << center << "|" << i << "|" << left_shift << std::endl;
        index = tree.indexes[center - i];
        PushCandidate(scoreboard, database.records[index], query, index);
    }
    for (uint32_t i = 1; i <= right_shift; i++) {
        if (database.length - center <= i)
            std::cout << "R whops: " << center << "|" << i << "|" << right_shift << std::endl;
        index = tree.indexes[center + i];
        PushCandidate(scoreboard, database.records[index], query, index);
    }

    if (scoreboard.size() != 101) {
        Debug("tfw");
    } else { 
        scoreboard.pop();
    }

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}

void RecursiveKDTreeSearch(Scoreboard& scoreboard,
                           const KDTree& tree,
                           const KDNode* node,
                           const Database& database,
                           const Query& query,
                           const uint32_t dim) {
    if (node != nullptr) {
        uint32_t index = tree.indexes[node->index];
        score_t score = distance(query, database.records[index]);
        float32_t delta = query.fields[dim] - node->median;

        if (scoreboard.size() < k_nearest_neighbors) {
            // std::cout << "fill-in := " << index << std::endl;
            scoreboard.push({
                .index = index,
                .score = score
            });
        } else if (score < scoreboard.top().score) {
            // std::cout << "fill-on := " << index << " => " << score << " instead of " << scoreboard.top().score << std::endl;
            scoreboard.pop();
            scoreboard.push({
                .index = index,
                .score = score
            });
        }

        if (node->type != LEAF) {
            const uint32_t next_dim = (dim + 1) % vector_num_dimension;
            if (delta > 0) {
                RecursiveKDTreeSearch(scoreboard, tree, node->right, database, query, next_dim);
                if (delta * delta < scoreboard.top().score
                    && scoreboard.size() < k_nearest_neighbors)
                    RecursiveKDTreeSearch(scoreboard, tree, node->left, database, query, next_dim);
            } else {
                RecursiveKDTreeSearch(scoreboard, tree, node->left, database, query, next_dim);
                if (delta * delta < scoreboard.top().score
                    && scoreboard.size() < k_nearest_neighbors)
                    RecursiveKDTreeSearch(scoreboard, tree, node->right, database, query, next_dim);
            }
        }
    }
}

void KDTreeSearch2(Result& result,
                   const KDTree& tree,
                   const Database& database,
                   const Query& query) {
    Scoreboard scoreboard;
    RecursiveKDTreeSearch(scoreboard, tree, tree.root, database, query, 0);

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}
