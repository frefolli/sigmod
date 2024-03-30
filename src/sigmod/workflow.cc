#include <sigmod/workflow.hh>
#include <sigmod/solution.hh>
#include <sigmod/query_set.hh>
#include <sigmod/database.hh>
#include <sigmod/seek.hh>
#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <queue>
#include <vector>
#include <algorithm>

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

inline score_t distance(const Query& query, const Record& record) {
    score_t sum = 0;
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        sum += pow(query.fields[i] - record.fields[i], 2);
    }
    return sum;
}

typedef struct {
    uint32_t index;
    score_t score;
} Candidate;

const auto compare_function = [](Candidate a, Candidate b) { return a.score < b.score; };
typedef std::priority_queue<Candidate, std::vector<Candidate>, decltype(compare_function)> Scoreboard;

inline void PushCandidate(Scoreboard& scoreboard, Record& record, Query& query, uint32_t record_index) {
    scoreboard.emplace(Candidate({
        .index = record_index,
        .score = distance(query, record)
    }));
}

enum query_t {
    NORMAL = 0,
    BY_C = 1,
    BY_T = 2,
    BY_C_AND_T = 3
};

inline void FilterIndexesByT(Database& database, uint32_t& start_index, uint32_t& end_index, float32_t l, float32_t r) {
    // it's guaranteed that the database is ordered by C, T, fields
    // in such "order"
    start_index = SeekHigh(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, l
    );

    end_index = SeekLow(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, r
    );
}

inline void FilterIndexesByC(c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, float32_t v) {
    start_index = C_map[v].first;
    end_index = C_map[v].second + 1;
}

inline void ExaustiveSearchByT(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        if (elegible_by_T(query, database.records[i])) {
            PushCandidate(scoreboard, database.records[i], query, i);
            if (scoreboard.size() > k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    }
}

inline void ExaustiveSearch(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        PushCandidate(scoreboard, database.records[i], query, i);
        if (scoreboard.size() > k_nearest_neighbors) {
            scoreboard.pop();
        }
    }
}

void FindForQuery(Result& result, Database& database, c_map_t& C_map, Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard(compare_function);

    const uint32_t query_type = (uint32_t) (query.query_type);

    uint32_t start_index = 0;
    uint32_t end_index = database.length;

    switch(query_type) {
        case BY_T: {
            for (auto C_it : C_map) {
                start_index = C_it.second.first;
                end_index = C_it.second.second;
                FilterIndexesByT(database, start_index, end_index, query.l, query.r);
                ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            }
            break;
        }; 
        case BY_C: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case BY_C_AND_T: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            FilterIndexesByT(database, start_index, end_index, query.l, query.r);
            ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case NORMAL: {
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
    }

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}

Solution SolveForQueries(Database& database,
                         c_map_t& C_map,
                         QuerySet& query_set) {
    Solution solution = {
        .length = query_set.length,
        .results = (Result*) malloc(sizeof(Result) * query_set.length)
    };
    for (uint32_t i = 0; i < query_set.length; i++) {
        FindForQuery(solution.results[i], database, C_map, query_set.queries[i]);
        #ifdef STOP_AFTER_1000
        if (i > 1000)
            break;
        #endif
    }
    return solution;
}

enum kdnode_t {
    MIDDLE = 0,
    LEAF = 1
};

struct KDNode {
    kdnode_t type;
    union {
        float32_t median;
        uint32_t index;
    };
    KDNode* left;
    KDNode* right;
};

struct KDTree {
    KDNode* root;
    uint32_t* indexes;
};

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
    KDNode* node = (KDNode*) malloc(sizeof(KDNode));
    if (start < end) {
        std::sort(indexes + start, indexes + end + 1, [&database, &dim](uint32_t a, uint32_t b) {
            return database.records[a].fields[dim] < database.records[b].fields[dim];
        });

        const uint32_t median = (start+end)/2;

        node->type = MIDDLE;
        node->median = database.records[indexes[median]].fields[dim];
        node->left = BuildKDNode(database, indexes, start, median, (dim + 1) % vector_num_dimension);
        node->right = BuildKDNode(database, indexes, median + 1, end, (dim + 1) % vector_num_dimension);
    } else if (start == end) {
        node->type = LEAF;
        node->index = start;
        node->left = nullptr;
        node->right = nullptr;
    } else {
        throw std::runtime_error("BuildKDNode! start > end");
    }
    return node;
}

KDTree BuildKDTree(Database& database) {
    #define EXPERIMENT_KDTREE
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }
    return {
        .root = BuildKDNode(database, indexes, 0, database.length - 1, 0),
        .indexes = indexes
    };
}

void KDTreeSearch(KDTree& tree, Database& database, Query& query, uint32_t query_index) {
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

    uint32_t kdtree = tree.indexes[current_node->index];

    /*
    Scoreboard scoreboard(compare_function);
    ExaustiveSearch(database, query, scoreboard, 0, database.length);

    uint32_t exaustive = 0;
    while(!scoreboard.empty()) {
        exaustive = scoreboard.top().index;
        scoreboard.pop();
    }

    if (exaustive != kdtree) {
        std::cout << query_index << " := " << exaustive << " vs " << kdtree << std::endl;
    }
    */
}

void Workflow(std::string database_path,
              std::string query_set_path,
              std::string output_path) {
    Database database = ReadDatabase(database_path);
    std::cout << "Read database, length = " << database.length << std::endl;
    QuerySet query_set = ReadQuerySet(query_set_path);
    std::cout << "Read query_set, length = " << query_set.length << std::endl;

    c_map_t C_map;
    IndexDatabase(database, C_map);
    
    #ifdef EXPERIMENT_KDTREE
    KDTree tree = BuildKDTree(database);
    for (uint32_t i = 0; i < query_set.length; i++) {
        KDTreeSearch(tree, database, query_set.queries[i], i);
    }
    FreeKDTree(tree);
    #else
    Solution solution = SolveForQueries(database, C_map, query_set);
    WriteSolution(solution, output_path);
    FreeSolution(solution);
    #endif

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
