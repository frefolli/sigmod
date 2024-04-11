#include <sigmod/mvp_tree.hh>
#include <sigmod/thread_pool.hh>

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <iostream>
#include <cassert>
#include <map>
#include <cfloat>

// main dependencies
// #include <sigmod/database.hh>
// #include <sigmod/query_set.hh>
// #include <sigmod/random.hh>
// #include <sigmod/debug.hh>
// #include <sigmod/flags.hh>

const uint32_t DATABASE_LENGTH = 1000000;
const uint32_t QUERYSET_LENGTH = 1;
uint32_t SIGMOD_MAX_LEVEL_REACHED = 0;
uint32_t SIGMOD_MALFORMED_LEAFS = 0;
std::map<uint32_t, uint32_t> SIGMOD_INTERNAL_BRANCH_USAGE = {};

void MVPNode::Free(MVPNode* node) {
  if (node != nullptr) {
    if (node->D1 != nullptr) {
      free(node->D1);
      node->D1 = nullptr;
    }
    if (node->D2 != nullptr) {
      free(node->D2);
      node->D2 = nullptr;
    }
    Free(node->C1);
    Free(node->C2);
    Free(node->C3);
    Free(node->C4);
    node->start = 0;
    node->end = 0;
    node->Sv1 = -1;
    node->Sv2 = -1;
    free(node);
  }
}

MVPNode* MVPNode::NewLeaf(uint32_t start, uint32_t end,
                        uint32_t Sv1, uint32_t Sv2,
                        score_t* D1, score_t* D2) {
  MVPNode* node = smalloc<MVPNode>();
  node->start = start;
  node->end = end;
  node->Sv1 = Sv1;
  node->Sv2 = Sv2;
  node->D1 = D1;
  node->D2 = D2;
  node->M1 = -1;
  node->M2 = -1;
  node->M3 = -1;
  node->C1 = nullptr;
  node->C2 = nullptr;
  node->C3 = nullptr;
  node->C4 = nullptr;
  node->type = MVPNode::LEAF;
  return node;
}

MVPNode* MVPNode::NewInternal(uint32_t start, uint32_t end,
                            uint32_t Sv1, uint32_t Sv2,
                            float32_t M1, float32_t M2, float32_t M3,
                            MVPNode* C1, MVPNode* C2, MVPNode* C3, MVPNode* C4) {
  MVPNode* node = smalloc<MVPNode>(1, "Leaf::Node");
  node->start = start;
  node->end = end;
  node->Sv1 = Sv1;
  node->Sv2 = Sv2;
  node->D1 = nullptr;
  node->D2 = nullptr;
  node->M1 = M1;
  node->M2 = M2;
  node->M3 = M3;
  node->C1 = C1;
  node->C2 = C2;
  node->C3 = C3;
  node->C4 = C4;
  node->type = MVPNode::INTERNAL;
  return node;
}

MVPNode* MVPTree::build_leaf(uint32_t start, uint32_t end, uint32_t length) {
  uint32_t Sv1 = start;
  score_t* D1 = smalloc<score_t>(length - 1, "Leaf::D1");
  uint32_t furthest_i = 0;
  float32_t furthest_d = distance(records[at(start + 1)], records[at(Sv1)]);
  D1[0] = furthest_d;
  for (uint32_t i = 1; i < length - 1; i++) {
    D1[i] = distance(records[at(start + 1 + i)], records[at(Sv1)]);
    if (D1[i] > furthest_d) {
      furthest_d = D1[i];
      furthest_i = i;
    }
  }
  if (furthest_i != 0) {
    std::swap(D1[furthest_i], D1[0]);
    std::swap(at(start + 1 + furthest_i), at(start + 1));
  }

  score_t* D2 = nullptr;
  uint32_t Sv2 = -1;
  if (length - 1 > 0) {
    Sv2 = start + 1;
    D2 = smalloc<score_t>(length - 2, "Leaf::D2");
    for (uint32_t i = 0; i < length - 2; i++) {
      D2[i] = distance(records[at(start + 2 + i)], records[at(start + 1)]);
    }
  }
  #ifndef TRACK_MALFORMED_LEAFS
  else {
      SIGMOD_MALFORMED_LEAFS += 1
  }
  #endif

  return MVPNode::NewLeaf(start, end, Sv1, Sv2, D1, D2);
}

MVPNode* MVPTree::build_internal(uint32_t start, uint32_t end, uint32_t length, uint32_t level) {
  uint32_t Sv1 = start;
  score_t* D1 = smalloc<score_t>(length - 1, "Internal::D1");
  for (uint32_t i = 0; i < length - 1; i++) {
    D1[i] = distance(records[at(start + 1 + i)], records[at(Sv1)]);
    if (level < this->p) {
        uint32_t p_index = at(start + 1 + i) * this->p;
      this->paths[p_index + level] = D1[i];
    }
  }
  ReorderByCoupledValue(indexes + start + 1, D1, length - 1);
  uint32_t median_index = (length - 1) / 2;
  float32_t M1 = D1[median_index];
  
  uint32_t SS1_start = start + 1;
  uint32_t SS1_end = start + 1 + median_index;
  uint32_t SS1_length = SS1_end - SS1_start;
  uint32_t SS2_start = start + 1 + median_index;
  uint32_t SS2_end = start + length;
  uint32_t SS2_length = SS2_end - SS2_start;

  uint32_t Sv2 = SS2_start;

  score_t* D2 = smalloc<score_t>(SS1_length, "Internal::D2");
  for (uint32_t i = 0; i < SS1_length; i++) {
    D2[i] = distance(records[at(SS1_start + i)],
                      records[at(Sv2)]);
    if (level + 1 < this->p) {
      uint32_t p_index = at(SS1_start + i) * this->p;
      this->paths[p_index + level + 1] = D2[i];
    }
  }
  ReorderByCoupledValue(indexes + SS1_start, D2, SS1_length);

  score_t* D3 = smalloc<score_t>(SS2_length - 1, "Internal::D3");
  for (uint32_t i = 0; i < SS2_length - 1; i++) {
    D3[i] = distance(records[at(SS2_start + 1 + i)],
                      records[at(Sv2)]);
    if (level + 1 < this->p) {
      uint32_t p_index = at(SS2_start + 1 + i) * this->p;
      this->paths[p_index + level + 1] = D3[i];
    }
  }
  ReorderByCoupledValue(indexes + SS2_start + 1, D3, SS2_length - 1);

  uint32_t SS1_median_index = SS1_length / 2;
  uint32_t SS2_median_index = (SS2_length - 1) / 2;
  float32_t M2 = D2[SS1_median_index];
  float32_t M3 = D3[SS2_median_index];

  uint32_t SS1_1_start = SS1_start;
  uint32_t SS1_1_end = SS1_start + SS1_median_index;
  uint32_t SS1_2_start = SS1_start + SS1_median_index;
  uint32_t SS1_2_end = SS1_end;

  uint32_t SS2_1_start = SS2_start + 1;
  uint32_t SS2_1_end = SS2_start + 1 + SS2_median_index;
  uint32_t SS2_2_start = SS2_start + 1 + SS2_median_index;
  uint32_t SS2_2_end = SS2_end;

  MVPNode* SS1_1_child = build_node(SS1_1_start, SS1_1_end, level + 2);
  MVPNode* SS1_2_child = build_node(SS1_2_start, SS1_2_end, level + 2);
  MVPNode* SS2_1_child = build_node(SS2_1_start, SS2_1_end, level + 2);
  MVPNode* SS2_2_child = build_node(SS2_2_start, SS2_2_end, level + 2);

  free(D1);
  free(D2);
  free(D3);

  return MVPNode::NewInternal(start, end,
                              Sv1, Sv2,
                              M1, M2, M3, 
                              SS1_1_child, SS1_2_child,
                              SS2_1_child, SS2_2_child);
}

MVPNode* MVPTree::build_node(uint32_t start, uint32_t end, uint32_t level) {
  if (start >= end)
    return nullptr;
  uint32_t length = end - start;
  if (length <= this->k + 2) {
    #ifdef TRACK_MAX_LEVEL_REACHED
    if (level > SIGMOD_MAX_LEVEL_REACHED) {
      SIGMOD_MAX_LEVEL_REACHED = level;
    }
    #endif
    return build_leaf(start, end, length);
  } else {
    #ifdef TRACK_MAX_LEVEL_REACHED
    if (level + 1 > SIGMOD_MAX_LEVEL_REACHED) {
      SIGMOD_MAX_LEVEL_REACHED = level + 1;
    }
    #endif
    return build_internal(start, end, length, level);
  }
}

void MVPTree::build(Record* records, uint32_t length, uint32_t* indexes) {
  this->k = MVPTree::OptimalK(k_nearest_neighbors);
  this->p = MVPTree::OptimalP(length);
  this->records = records;
  this->length = length;
  this->indexes = indexes;
  this->paths = smalloc<score_t>(this->length * this->p);
  for (uint32_t i = 0; i < this->length; i++)
      this->paths[i] = -1;
  this->root = this->build_node(0, this->length, 0);
}

void MVPTree::knn_search_leaf(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
  const uint32_t Sv1 = at(node->Sv1);
  const uint32_t Sv2 = at(node->Sv2);

  const float32_t dSv1 = distance(q, records[Sv1]);
  const float32_t dSv2 = distance(q, records[Sv2]);

  if (dSv1 <= r) {
    scoreboard.push(Sv1, dSv1);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }
  if (dSv2 <= r) {
    scoreboard.push(Sv2, dSv2);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }

  uint32_t length = node->end - node->start;
  for (uint32_t i = 0; i < length - 2; i++) {
    const uint32_t index = at(node->start + 2 + i);
    if (scoreboard.not_full()) {
      const float32_t diq = distance(q, records[i]);
      scoreboard.push(i, diq);
      if (scoreboard.full())
        r = scoreboard.furthest().score;
    } else {
      if (std::fabs(dSv1 - node->D1[i + 1]) <= r) {
          if (std::fabs(dSv2 - node->D2[i]) <= r) {
            bool compute_d = true;
            const uint32_t p_index = index * p;
            for (uint32_t j = 0; j < level; j++) {
              if (!(std::fabs(PATH[j] - paths[p_index + j]) <= r)) {
                  compute_d = false;
              }
            }
            if (compute_d) {
              const float32_t diq = distance(q, records[index]);
              if (diq <= r) {
                scoreboard.push(index, diq);
                r = scoreboard.furthest().score;
              }
            }
        }
      }
    }
  }
}

void MVPTree::knn_search_internal(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
  const uint32_t Sv1 = at(node->Sv1);
  const uint32_t Sv2 = at(node->Sv2);

  const float32_t dSv1 = distance(q, records[Sv1]);
  const float32_t dSv2 = distance(q, records[Sv2]);

  if (dSv1 <= r) {
    scoreboard.push(Sv1, dSv1);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }
  if (dSv2 <= r) {
    scoreboard.push(Sv2, dSv2);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }

  if (level < p) {
    PATH[level] = dSv1;
  }
  if (level + 1 < p) {
    PATH[level + 1] = dSv2;
  }

  if (dSv1 - r <= node->M1) {
    if (dSv2 - r <= node->M2 && node->C1 != nullptr) {
      knn_search(q, scoreboard, PATH, r, level + 2, node->C1);
    }
    if (dSv2 + r >= node->M2 && node->C2 != nullptr) {
      knn_search(q, scoreboard, PATH, r, level + 2, node->C2);
    }
  }
  if (dSv1 + r >= node->M1) {
    if (dSv2 - r <= node->M3 && node->C3 != nullptr) {
      knn_search(q, scoreboard, PATH, r, level + 2, node->C3);
    }
    if (dSv2 + r >= node->M3 && node->C4 != nullptr) {
      knn_search(q, scoreboard, PATH, r, level + 2, node->C4);
    }
  }
}

void MVPTree::knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
  if (node->type == MVPNode::LEAF) {
    knn_search_leaf(q, scoreboard, PATH, r, level, node);
  } else {
    knn_search_internal(q, scoreboard, PATH, r, level, node);
  }
}

void MVPTree::knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH) {
  float32_t r = FLT_MAX;
  if (scoreboard.full())
    r = scoreboard.furthest().score;
  knn_search(q, scoreboard, PATH, r, 0, root);
}

uint32_t MVPTree::OptimalK(uint32_t n_of_nearest_neighbors) {
  return n_of_nearest_neighbors;
}

uint32_t MVPTree::OptimalP(uint32_t n_of_records) {
  static const float32_t a = 1.43027086e+00;
  static const float32_t b = 4.44754191e-01;
  static const float32_t c = 1.94581988e+03;
  static const float32_t d = -3.57875462e+00;
  static const float32_t y = a * std::log(b * n_of_records + c) + d;
  return std::round(y);
}

MVPTree MVPTree::New() {
  return {
    .records = nullptr,
    .length = 0,
    .indexes = nullptr,
    .root = nullptr,
    .paths = nullptr,
    .k = 1,
    .p = 1
  };
}

void MVPTree::Free(MVPTree& tree) {
  if (tree.records != nullptr) {
    tree.records = nullptr;
    tree.length = 0;
  }
  if (tree.indexes != nullptr) {
    free(tree.indexes);
    tree.indexes = nullptr;
  }
  if (tree.root != nullptr) {
    MVPNode::Free(tree.root);
    tree.root = nullptr;
  }
  if (tree.paths != nullptr) {
    free(tree.paths);
    tree.paths = nullptr;
  }
}

void MVPTree::Check(MVPTree& tree, MVPNode* node) {
  if (node != nullptr) {
    if (node->type == MVPNode::LEAF) {
    } else {
      uint32_t length = node->end - node->start;
      uint32_t C1_length = node->C1->end - node->C1->start;
      uint32_t C2_length = node->C2->end - node->C2->start;
      uint32_t C3_length = node->C3->end - node->C3->start;
      uint32_t C4_length = node->C4->end - node->C4->start;
      if (length != (C1_length + C2_length + C3_length + C4_length + 2)
          || (node->C1->start != node->start + 1)
          || (node->C2->start != node->C1->end)
          || (node->C3->start != node->C2->end + 1)
          || (node->C4->start != node->C3->end)) {
        std::string error_message = "inconsistency! ";
        error_message += "[" + std::to_string(node->start) + "," + std::to_string(node->end) + ")";
        error_message += " is mapped to {";
        error_message += "[" + std::to_string(node->C1->start) + "," + std::to_string(node->C1->end) + ") * ";
        error_message += "[" + std::to_string(node->C2->start) + "," + std::to_string(node->C2->end) + ") * ";
        error_message += "[" + std::to_string(node->C3->start) + "," + std::to_string(node->C3->end) + ") * ";
        error_message += "[" + std::to_string(node->C4->start) + "," + std::to_string(node->C4->end) + ")";
        error_message += "}";
        throw std::runtime_error(error_message);
      }
      for (uint32_t i = 0; i < C1_length; i++) {
          float32_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C1->start + i)]);
          float32_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C1->start + i)]);
          assert(d1 <= node->M1);
          assert(d2 <= node->M2);
      }
      for (uint32_t i = 0; i < C2_length; i++) {
          float32_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C2->start + i)]);
          float32_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C2->start + i)]);
          assert(d1 <= node->M1);
          assert(d2 >= node->M2);
      }
      for (uint32_t i = 0; i < C3_length; i++) {
          float32_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C3->start + i)]);
          float32_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C3->start + i)]);
          assert(d1 >= node->M1);
          assert(d2 <= node->M3);
      }
      for (uint32_t i = 0; i < C4_length; i++) {
          float32_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C4->start + i)]);
          float32_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C4->start + i)]);
          assert(d1 >= node->M1);
          assert(d2 >= node->M3);
      }
      Check(tree, node->C1);
      Check(tree, node->C2);
      Check(tree, node->C3);
      Check(tree, node->C4);
    }
  }
}

void MVPTree::Check(MVPTree& tree) {
  Check(tree, tree.root);
}

MVPTree MVPTree::Build(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
  MVPTree tree = MVPTree::New();
  tree.build(database.records + start, end - start, indexes);
  return tree;
}

MVPForest MVPForest::Build(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    std::map<uint32_t, MVPTree> trees;
    #ifdef CONCURRENCY
    std::mutex mutex;
    ThreadPool pool;
    pool.run([&trees, &database, &indexes, &mutex](typename c_map_t::const_iterator cat) {
        BallTree tree = MVPTree::Build(database, indexes, cat->second.first, cat->second.second + 1);
        std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
        trees[cat->first] = tree;
        delete guard;
    }, database.C_map);
    assert (trees.size() == database.C_map.size());
    #else
    for (auto cat : database.C_map) {
        trees[cat.first] = MVPTree::Build(database, indexes, cat.second.first, cat.second.second + 1);
    }
    #endif

    return {
        .indexes = indexes,
        .trees = trees
    };
}

void MVPForest::knn_search(const Database& database, Result& result, const Query& query) {
  Scoreboard gboard;

  #ifdef DISATTEND_CHECKS
  const uint32_t query_type = NORMAL;
  #else
  const uint32_t query_type = (uint32_t) (query.query_type);
  #endif

  score_t* PATH = smalloc<score_t>(40); // TODO: segnati max_p
  if (query_type == BY_C) {
    // trees.at(query.v).knn_search(query, gboard, PATH);
  } else if (query_type == BY_C_AND_T) {
    // trees.at(query.v).knn_search(query, gboard, PATH);
  } else if (query_type == BY_T) {
    for (auto tree : trees) {
      // tree.second.knn_search(query, gboard, PATH);
    }
  } else {
    for (auto tree : trees) {
      // tree.second.knn_search(query, gboard, PATH);
    }
  }
  free(PATH);

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

/*

#include <chrono>
#include <filesystem>
#include <sigmod.hh>
#include <iostream>
#include <stdexcept>

void assert_file_exists(std::string path, std::string what) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error(what + ": path '" + path + "' doesn't exists");
  }
}

int main(int argc, char** args) {
    // std::srand(std::time(0));

    std::string database_path = "../../dummy-data.bin";
    std::string query_set_path = "../../dummy-queries.bin";
    std::string output_path = "output.bin";

    if (argc > 1) {
        database_path = std::string(args[1]);
    }

    if (argc > 2) {
        query_set_path = std::string(args[2]);
    }

    if (argc > 3) {
        output_path = std::string(args[3]);
    }

    #ifdef READ_DATABASE_FROM_FILE
    assert_file_exists(database_path, "database_path");
    Database database = ReadDatabase(database_path);
    LogTime("Read DB");
    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Read DB");
    #endif
    #else
    Database database = {
      .length = DATABASE_LENGTH,
      .records = smalloc<Record>(DATABASE_LENGTH, "database")
    };
    RandomizeDatabase(database);
    LogTime("Created DB");
    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Created DB");
    #endif
    #endif

    #ifdef READ_QUERYSET_FROM_FILE
    assert_file_exists(query_set_path, "query_set_path");
    QuerySet queryset = ReadQuerySet(query_set_path);
    LogTime("Read QS");
    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Read QS");
    #endif
    #else
    QuerySet queryset = {
      .length = QUERYSET_LENGTH,
      .queries = smalloc<Query>(QUERYSET_LENGTH, "queryset")
    };
    RandomizeQuerySet(queryset);
    LogTime("Created QS");
    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Created QS");
    #endif
    #endif

    uint32_t n_of_queries = std::min(QUERYSET_LENGTH, queryset.length);

    std::cout << "Length = " << database.length << std::endl;
    std::cout << "Queries = " << queryset.length << std::endl;
    std::cout << "n_of_queries = " << n_of_queries << std::endl;
    std::cout << "actual_vector_size = " << actual_vector_size << std::endl;
    std::cout << "k_nearest_neighbors = " << k_nearest_neighbors << std::endl;
    LogTime("Start");

    MVPTree tree = MVPTree::New();
    tree.build(database.records, database.length);
    LogTime("Built Tree");
    
    #ifdef CHECK_MVP_TREE
    LogTime("Tree::P := " + std::to_string(tree.p));
    LogTime("Tree::K := " + std::to_string(tree.k));
    MVPTree::Check(tree);
    LogTime("Checked Tree");

    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Built Tree");
    #endif

    #ifdef TRACK_MAX_LEVEL_REACHED
    LogTime("Max Level Reached := " + std::to_string(SIGMOD_MAX_LEVEL_REACHED));
    #endif

    #ifdef TRACK_MALFORMED_LEAFS
    LogTime("Num of Malformed Leafs := " + std::to_string(SIGMOD_MALFORMED_LEAFS));
    #endif
    #endif

    SIGMOD_DISTANCE_COMPUTATIONS = 0;
    Scoreboard scoreboard;
    score_t* PATH = smalloc<score_t>(tree.p);
    for (uint32_t i = 0; i < n_of_queries; i++) {
      scoreboard.clear();
      tree.knn_search(queryset.queries[i], scoreboard, PATH);
    }
    LogTime("Queried Tree! k_nearest_neighbors");
    free(PATH);
    
    #ifdef TRACK_DISTANCE_COMPUTATIONS
    LogTime("DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS));
    LogTime("Mean DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS / queryset.length));
    #endif

    SIGMOD_DISTANCE_COMPUTATIONS = 0;
    for (uint32_t i = 0; i < n_of_queries; i++) {
        scoreboard.clear();
        for (uint32_t j = 0; j < database.length; j++) {
            float32_t d = distance(queryset.queries[i], database.records[j]);
            scoreboard.push(j, d);
        }
    }
    LogTime("Queried Exaustive! k_nearest_neighbors");
    
    #ifdef TRACK_DISTANCE_COMPUTATIONS
    LogTime("DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS));
    LogTime("Mean DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS / queryset.length));
    #endif

    MVPTree::Free(tree);
    LogTime("Cleared Tree");
  
    FreeDatabase(database);
    LogTime("Cleared DB");

    FreeQuerySet(queryset);
    LogTime("Cleared QS");
    
    LogTime("End");
}

*/