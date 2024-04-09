#include <sigmod/kd_tree.hh>
#include <sigmod/flags.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/debug.hh>
#include <sigmod/random.hh>
#include <algorithm>

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

uint32_t MaximizeSpread(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
  uint32_t best_dim = 0;
  float32_t cur_min = database.at(start).fields[best_dim];
  float32_t cur_max = database.at(start).fields[best_dim];
  for (uint32_t i = start + 1; i <= end; i++) {
    const float32_t val = database.at(indexes[i]).fields[best_dim];
    if (val < cur_min)
        cur_min = val;
    if (val > cur_max)
        cur_max = val;
  }

  float32_t best_min = cur_min;
  float32_t best_max = cur_max;

  for (uint32_t dim = 1; dim < actual_vector_size; dim++) {
    cur_min = database.at(start).fields[dim];
    cur_max = database.at(start).fields[dim];
    for (uint32_t i = start + 1; i <= end; i++) {
      const float32_t val = database.at(indexes[i]).fields[best_dim];
      if (val < cur_min)
          cur_min = val;
      if (val > cur_max)
          cur_max = val;
    }
    if (cur_max - cur_min > best_max - best_min) {
      best_min = cur_min;
      best_max = cur_max;
      best_dim = dim;
    }
  }

  return best_dim;
}

// for interval [start, end]
// note that end is included
KDNode* BuildKDNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t dim) {
    KDNode* node = (KDNode*) malloc(sizeof(KDNode));
    if (node == nullptr)
        throw std::runtime_error("insufficient memory");

    std::sort(indexes + start, indexes + end + 1, [&database, &dim](uint32_t a, uint32_t b) {
        return database.at(a).fields[dim] < database.at(b).fields[dim];
    });

    const uint32_t median = (start + end)/2;
    node->value = database.at(indexes[median]).fields[dim];
    node->index = median;
    node->dim = dim;
    
    if (median-1 != end && start <= median-1 && median-1 <= database.length) {
        #ifdef KD_FOREST_DIMENSION_RANDOMIZE
        const uint32_t next_dim = RandomUINT32T(0, actual_vector_size);
        #else
          #ifdef KD_FOREST_DIMENSION_MAXIMIZE_SPREAD
          const uint32_t next_dim = MaximizeSpread(database, indexes, start, median - 1);
          #else
          const uint32_t next_dim = (dim + 1) % actual_vector_size;
          #endif
        #endif
        node->left = BuildKDNode(database, indexes, start, median-1, next_dim);
    } else {
        node->left = nullptr;
    }
    if (median + 1 != start && median + 1 <= end && end <= database.length) {
        #ifdef KD_FOREST_DIMENSION_RANDOMIZE
        const uint32_t next_dim = RandomUINT32T(0, actual_vector_size);
        #else
          #ifdef KD_FOREST_DIMENSION_MAXIMIZE_SPREAD
          const uint32_t next_dim = MaximizeSpread(database, indexes, median + 1, end);
          #else
          const uint32_t next_dim = (dim + 1) % actual_vector_size;
          #endif
        #endif
        node->right = BuildKDNode(database, indexes, median + 1, end, next_dim);
    } else {
        node->right = nullptr;
    }
    return node;
}

KDTree BuildKDTree(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t first_dim) {
    #ifdef KD_FOREST_DIMENSION_RANDOMIZE
    const uint32_t next_dim = RandomUINT32T(0, actual_vector_size);
    #else
      #ifdef KD_FOREST_DIMENSION_MAXIMIZE_SPREAD
      const uint32_t next_dim = MaximizeSpread(database, indexes, start, end);
      #else
      const uint32_t next_dim = first_dim;
      #endif
    #endif

    return {
        .root = BuildKDNode(database, indexes, start, end, next_dim),
        .indexes = indexes
    };
}

KDForest BuildKDForest(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, KDTree> trees;
    for (auto cat : database.C_map) {
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
    const uint32_t index = tree.indexes[node->index];
    const score_t score = distance(query, database.at(index));
    const float32_t delta = query.fields[node->dim] - node->value;

    #ifndef DISATTEND_CHECKS
      if (check_if_elegible_by_T(query, database.at(index)))
    #endif
    scoreboard.push(index, score);

    if (!IsLeaf(node)) {
        if (delta > 0) {
            if (node->right != nullptr)
                SearchKDNode(database, query, scoreboard, tree, node->right);
            if (node->left != nullptr && (scoreboard.empty() || delta * delta < scoreboard.top().score))
                SearchKDNode(database, query, scoreboard, tree, node->left);
        } else {
            if (node->left != nullptr)
                SearchKDNode(database, query, scoreboard, tree, node->left);
            if (node->right != nullptr && (scoreboard.empty() || delta * delta < scoreboard.top().score))
                SearchKDNode(database, query, scoreboard, tree, node->right);
        }
    }
}

void SearchKDTree(const Database& database, const Query& query, Scoreboard& scoreboard, const KDTree& tree) {
    SearchKDNode(database, query, scoreboard, tree, tree.root);
}

void SearchKDForest(const KDForest& forest, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;

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
        result.data[rank] = database.indexes[gboard.top().index];
        gboard.pop();
        rank--;
    }
}
