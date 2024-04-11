#include <sigmod/ball_tree.hh>
#include <sigmod/flags.hh>
#include <sigmod/tweaks.hh>
#include <sigmod/debug.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/thread_pool.hh>
#include <sigmod/tree_utils.hh>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cfloat>

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
    forest.trees = {};
    if (forest.indexes != nullptr) {
        free(forest.indexes);
        forest.indexes = nullptr;
    }
}

bool IsLeaf(const BallNode* node) {
    return (node->left == nullptr && node->right == nullptr);
}

inline bool is_leftist(const Database& database, const uint32_t* indexes, const uint32_t a, const uint32_t b, const uint32_t x) {
    const score_t da = distance(database.at(indexes[a]), database.at(indexes[x]));
    const score_t db = distance(database.at(indexes[b]), database.at(indexes[x]));
    return (da - db) < 0;
}

BallNode* BuildBallNode(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
    assert(start < end && end <= database.length);

    if (end - start <= BALL_NODE_SIZE) {
        BallNode* node = (BallNode*) malloc (sizeof(BallNode));
        node->start = start;
        node->end = end;
        const uint32_t length = end - start;
        for (uint32_t i = 0; i < actual_vector_size; i++) {
            node->center.fields[i] = database.at(indexes[start]).fields[i] / length;
        }
        for (uint32_t j = start + 1; j < end; j++) {
            for (uint32_t i = 0; i < actual_vector_size; i++) {
                node->center.fields[i] += database.at(indexes[j]).fields[i] / length;
            }
        }
        node->radius = 0;
        for (uint32_t j = start; j < end; j++) {
            score_t radius = distance(node->center, database.at(indexes[j]));
            if (radius > node->radius)
                node->radius = radius;
        }
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }

    uint32_t a = FindFurthestPoint(database, indexes, start, end, start);
    uint32_t b = FindFurthestPoint(database, indexes, start, end, a);
    
    if (a != start) {
        std::swap(indexes[a], indexes[start]);
        a = start;
    }
    if (b != end - 1) {
        std::swap(indexes[b], indexes[end - 1]);
        b = end - 1;
    }

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

    const uint32_t next_start = i;
    const uint32_t next_end = i;

    BallNode* node = (BallNode*) malloc (sizeof(BallNode));
    node->start = start;
    node->end = end;

    node->left = BuildBallNode(database, indexes, start, next_end);
    node->right = BuildBallNode(database, indexes, next_start, end);

    assert(node->left != nullptr && node->right != nullptr);
    
    // const float32_t alpha = (node->left->end - node->left->start) / (node->end - node->start);
    // const float32_t beta = (node->right->end - node->right->start) / (node->end - node->start);
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        node->center.fields[i] = (node->left->center.fields[i] + node->right->center.fields[i]) / 2;
        // node->center.fields[i] = alpha * node->left->center.fields[i] + beta * node->right->center.fields[i];
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

BallForest BuildBallForest(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, BallTree> trees;
    #ifdef CONCURRENCY
    std::mutex mutex;
    ThreadPool pool;
    pool.run([&trees, &database, &indexes, &mutex](typename c_map_t::const_iterator cat) {
        BallTree tree = BuildBallTree(database, indexes, cat->second.first, cat->second.second + 1);
        std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
        trees[cat->first] = tree;
        delete guard;
    }, database.C_map);
    assert (trees.size() == database.C_map.size());
    #else
    for (auto cat : database.C_map) {
        trees[cat.first] = BuildBallTree(database, indexes, cat.second.first, cat.second.second + 1);
    }
    #endif

    return {
        .indexes = indexes,
        .trees = trees
    };
}

void SearchBallNode(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, score_t& Dsofar, const score_t Dminp) {
    if (IsLeaf(node)) {
        for (uint32_t i = node->start; i < node->end; i++) {
            const uint32_t p = tree.indexes[i];
            const score_t distance_query_p = distance(query, database.at(p));
            scoreboard.push(p, distance_query_p);
            if (scoreboard.full())
                Dsofar = scoreboard.top().score;
        }
    } else {
        const score_t distance_query_left = distance(query, node->left->center);
        const score_t distance_query_right = distance(query, node->right->center);

        const score_t LDminp = std::max(distance_query_left - node->left->radius, Dminp);
        const score_t RDminp = std::max(distance_query_right - node->right->radius, Dminp);

        if (distance_query_left < distance_query_right) {
            if (LDminp < Dsofar)
                SearchBallNode(database, query, scoreboard, tree, node->left, Dsofar, LDminp);
            if (RDminp < Dsofar)
                SearchBallNode(database, query, scoreboard, tree, node->right, Dsofar, RDminp);
        } else {
            if (RDminp < Dsofar)
                SearchBallNode(database, query, scoreboard, tree, node->right, Dsofar, RDminp);
            if (LDminp < Dsofar)
                SearchBallNode(database, query, scoreboard, tree, node->left, Dsofar, LDminp);
        }
    }
}

void SearchBallNodeByT(const Database& database, const Query& query,
                    Scoreboard& scoreboard, const BallTree& tree,
                    const BallNode* node, score_t& Dsofar, const score_t Dminp) {
    if (IsLeaf(node)) {
        for (uint32_t i = node->start; i < node->end; i++) {
            const uint32_t p = tree.indexes[i];
            #ifndef DISATTEND_CHECKS
              if (!elegible_by_T(query, database.at(p)))
                continue;
            #endif
            const score_t distance_query_p = distance(query, database.at(p));
            scoreboard.push(p, distance_query_p);
            if (scoreboard.full())
                Dsofar = scoreboard.top().score;
        }
    } else {
        const score_t distance_query_left = distance(query, node->left->center);
        const score_t distance_query_right = distance(query, node->right->center);

        const score_t LDminp = std::max(distance_query_left - node->left->radius, Dminp);
        const score_t RDminp = std::max(distance_query_right - node->right->radius, Dminp);

        if (distance_query_left < distance_query_right) {
            if (LDminp < Dsofar)
                SearchBallNodeByT(database, query, scoreboard, tree, node->left, Dsofar, LDminp);
            if (RDminp < Dsofar)
                SearchBallNodeByT(database, query, scoreboard, tree, node->right, Dsofar, RDminp);
        } else {
            if (RDminp < Dsofar)
                SearchBallNodeByT(database, query, scoreboard, tree, node->right, Dsofar, RDminp);
            if (LDminp < Dsofar)
                SearchBallNodeByT(database, query, scoreboard, tree, node->left, Dsofar, LDminp);
        }
    }
}

void SearchBallTree(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree) {
    const score_t distance_query_root = distance(query, tree.root->center);
    const score_t Dminp = std::max(distance_query_root - tree.root->radius, (score_t) 0);
    score_t Dsofar = DBL_MAX;
    if (scoreboard.full())
        Dsofar = scoreboard.top().score;
    if (Dminp < Dsofar)
        SearchBallNode(database, query, scoreboard, tree, tree.root, Dsofar, Dminp);
}

void SearchBallTreeByT(const Database& database, const Query& query, Scoreboard& scoreboard, const BallTree& tree) {
    const score_t distance_query_root = distance(query, tree.root->center);
    const score_t Dminp = std::max(distance_query_root - tree.root->radius, (score_t) 0);
    score_t Dsofar = DBL_MAX;
    if (scoreboard.full())
        Dsofar = scoreboard.top().score;
    if (Dminp < Dsofar)
        SearchBallNodeByT(database, query, scoreboard, tree, tree.root, Dsofar, Dminp);
}

void SearchBallForest(const BallForest& forest, const Database& database, Result& result, const Query& query) {
    Scoreboard gboard;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    if (query_type == BY_C) {
        SearchBallTree(database, query, gboard, forest.trees.at(query.v));
    } else if (query_type == BY_C_AND_T) {
        SearchBallTreeByT(database, query, gboard, forest.trees.at(query.v));
    } else if (query_type == BY_T) {
        for (auto tree : forest.trees) {
            SearchBallTreeByT(database, query, gboard, tree.second);
        }
    } else {
        for (auto tree : forest.trees) {
            SearchBallTree(database, query, gboard, tree.second);
        }
    }

    assert (gboard.full());
    uint32_t rank = gboard.size() - 1;
    while(!gboard.empty()) {
        #ifdef FAST_INDEX
        result.data[rank] = gboard.top().index;
        #else
        result.data[rank] = database.indexes[gboard.top().index];
        #endif
        gboard.pop();
        rank--;
    }
}
