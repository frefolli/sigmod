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
#include <unordered_map>
#include <cfloat>

const uint32_t DATABASE_LENGTH = 1000000;
const uint32_t QUERYSET_LENGTH = 1;
uint32_t SIGMOD_MAX_LEVEL_REACHED = 0;
uint32_t SIGMOD_MALFORMED_LEAFS = 0;
std::unordered_map<uint32_t, uint32_t> SIGMOD_INTERNAL_BRANCH_USAGE = {};

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
                            score_t M1, score_t M2, score_t M3,
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
  uint32_t Sv2 = -1;
  score_t* D1 = nullptr;
  score_t* D2 = nullptr;
  if (length - 1 > 0) {
      D1 = smalloc<score_t>(length - 1, "Leaf::D1");
      uint32_t furthest_i = 0;
      score_t furthest_d = distance(records[at(start + 1)], records[at(Sv1)]);
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

      Sv2 = start + 1;
      if (length - 2 > 0) {
        D2 = smalloc<score_t>(length - 2, "Leaf::D2");
        for (uint32_t i = 0; i < length - 2; i++) {
          D2[i] = distance(records[at(start + 2 + i)], records[at(Sv2)]);
        }
      }
      #ifndef TRACK_MALFORMED_LEAFS
      else {
          SIGMOD_MALFORMED_LEAFS += 1
      }
      #endif
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
      uint32_t p_index = pindex(start + 1 + i);
      this->paths[p_index + level] = D1[i];
    }
  }
  ReorderByCoupledValue(indexes + start + 1, D1, length - 1);
  uint32_t median_index = (length - 1) / 2;
  score_t M1 = D1[median_index];
  
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
      uint32_t p_index = pindex(SS1_start + i);
      this->paths[p_index + level + 1] = D2[i];
    }
  }
  ReorderByCoupledValue(indexes + SS1_start, D2, SS1_length);

  score_t* D3 = smalloc<score_t>(SS2_length - 1, "Internal::D3");
  for (uint32_t i = 0; i < SS2_length - 1; i++) {
    D3[i] = distance(records[at(SS2_start + 1 + i)],
                      records[at(Sv2)]);
    if (level + 1 < this->p) {
      uint32_t p_index = pindex(SS2_start + 1 + i);
      this->paths[p_index + level + 1] = D3[i];
    }
  }
  ReorderByCoupledValue(indexes + SS2_start + 1, D3, SS2_length - 1);

  uint32_t SS1_median_index = SS1_length / 2;
  uint32_t SS2_median_index = (SS2_length - 1) / 2;
  score_t M2 = D2[SS1_median_index];
  score_t M3 = D3[SS2_median_index];

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

void MVPTree::build(Record* records, uint32_t length, uint32_t* indexes, score_t* paths, uint32_t max_p) {
  this->k = MVPTree::OptimalK(k_nearest_neighbors);
  this->p = MVPTree::OptimalP(length);
  this->max_p = max_p;
  this->records = records;
  this->length = length;
  this->indexes = indexes;
  this->paths = paths;
  this->root = this->build_node(0, this->length, 0);
}

uint32_t MVPTree::OptimalK(uint32_t n_of_nearest_neighbors) {
  return n_of_nearest_neighbors * 2;
}

uint32_t MVPTree::OptimalP(uint32_t n_of_records) {
  static const score_t a = 1.43027086e+00;
  static const score_t b = 4.44754191e-01;
  static const score_t c = 1.94581988e+03;
  static const score_t d = -3.57875462e+00;
  static const score_t y = a * std::log(b * n_of_records + c) + d;
  return std::round(y);
}

void MVPTree::knn_search_leaf(const Query& q, Scoreboard& scoreboard, score_t* PATH, score_t& r, uint32_t level, const MVPNode* node) const {
  const uint32_t Sv1 = at(node->Sv1);
  const score_t dSv1 = distance(q, records[Sv1]);
  if (check_if_elegible_by_T(q, records[Sv1]) && dSv1 <= r) {
    scoreboard.pushf(Sv1, dSv1);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }

  if (node->D1 == nullptr)
      return;

  const uint32_t Sv2 = at(node->Sv2);
  const score_t dSv2 = distance(q, records[Sv2]);
  if (check_if_elegible_by_T(q, records[Sv2]) && dSv2 <= r) {
    scoreboard.pushf(Sv2, dSv2);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }

  if (node->D2 == nullptr)
      return;

  uint32_t length = node->end - node->start;
  for (uint32_t i = 0; i < length - 2; i++) {
    const uint32_t index = at(node->start + 2 + i);
    const uint32_t p_index = pindex(node->start + 2 + i);
    if (!check_if_elegible_by_T(q, records[index]))
        continue;
    if (scoreboard.not_full()) {
      const score_t diq = distance(q, records[index]);
      scoreboard.pushf(index, diq);
      if (scoreboard.full())
        r = scoreboard.furthest().score;
    } else {
      if (std::fabs(dSv1 - node->D1[i + 1]) <= r) {
          if (std::fabs(dSv2 - node->D2[i]) <= r) {
            bool compute_d = true;
            for (uint32_t j = 0; j < level && j < p; j++) {
              if (std::fabs(PATH[j] - paths[p_index + j]) > r) {
                  compute_d = false;
                  break;
              }
            }
            if (compute_d) {
              const score_t diq = distance(q, records[index]);
              if (diq <= r) {
                scoreboard.pushf(index, diq);
                r = scoreboard.furthest().score;
              }
            }
        }
      }
    }
  }
}

void MVPTree::knn_search_internal(const Query& q, Scoreboard& scoreboard, score_t* PATH, score_t& r, uint32_t level, const MVPNode* node) const {
 const  uint32_t Sv1 = at(node->Sv1);
 const  uint32_t Sv2 = at(node->Sv2);

  const score_t dSv1 = distance(q, records[Sv1]);
  const score_t dSv2 = distance(q, records[Sv2]);

  if (check_if_elegible_by_T(q, records[Sv1]) && dSv1 <= r) {
    scoreboard.pushf(Sv1, dSv1);
    if (scoreboard.full())
      r = scoreboard.furthest().score;
  }
  if (check_if_elegible_by_T(q, records[Sv2]) && dSv2 <= r) {
    scoreboard.pushf(Sv2, dSv2);
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

void MVPTree::knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH, score_t& r, uint32_t level, const MVPNode* node) const {
  if (node->type == MVPNode::LEAF) {
    knn_search_leaf(q, scoreboard, PATH, r, level, node);
  } else {
    knn_search_internal(q, scoreboard, PATH, r, level, node);
  }
}

void MVPTree::knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH) const {
  score_t r = DBL_MAX;
  if (scoreboard.full())
    r = scoreboard.furthest().score;
  knn_search(q, scoreboard, PATH, r, 0, root);
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
    tree.indexes = nullptr;
  }
  if (tree.root != nullptr) {
    MVPNode::Free(tree.root);
    tree.root = nullptr;
  }
  if (tree.paths != nullptr) {
    tree.paths = nullptr;
  }
}

void MVPTree::Check(MVPTree& tree, MVPNode* node) {
  if (node != nullptr) {
    if (node->type == MVPNode::LEAF) {
        if (node->D1 != nullptr) {
            uint32_t length = node->end - node->start;
            for (uint32_t i = 0; i < length - 1; i++) {
                assert(node->D1[i] == distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->start + 1 + i)]));
            }
            if (node->D2 != nullptr) {
                for (uint32_t i = 0; i < length - 2; i++) {
                    assert(node->D2[i] == distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->start + 2 + i)]));
                }
            }
        }
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
          score_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C1->start + i)]);
          score_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C1->start + i)]);
          assert(d1 <= node->M1);
          assert(d2 <= node->M2);
      }
      for (uint32_t i = 0; i < C2_length; i++) {
          score_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C2->start + i)]);
          score_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C2->start + i)]);
          assert(d1 <= node->M1);
          assert(d2 >= node->M2);
      }
      for (uint32_t i = 0; i < C3_length; i++) {
          score_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C3->start + i)]);
          score_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C3->start + i)]);
          assert(d1 >= node->M1);
          assert(d2 <= node->M3);
      }
      for (uint32_t i = 0; i < C4_length; i++) {
          score_t d1 = distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C4->start + i)]);
          score_t d2 = distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C4->start + i)]);
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

MVPTree MVPTree::Build(const Database& database, score_t* paths, uint32_t p, uint32_t* indexes, const uint32_t start, const uint32_t end) {
  MVPTree tree = MVPTree::New();
  tree.build(database.records, end - start, indexes + start, paths, p);
  return tree;
}

MVPForest MVPForest::Build(const Database& database) {
    uint32_t* indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
        indexes[i] = i;
    }

    uint32_t max_p = 0;
    for (auto cat : database.C_map) {
        uint32_t p = MVPTree::OptimalP(cat.second.second + 1 - cat.second.first);
        if (p > max_p)
            max_p = p;
    }

    uint32_t paths_size = max_p * database.length;
    score_t* paths = smalloc<score_t>(paths_size);
    for (uint32_t i = 0; i < paths_size; i++)
        paths[i] = -1;

    std::unordered_map<uint32_t, MVPTree> trees;
    #ifdef CONCURRENCY
    std::mutex mutex;
    ThreadPool pool;
    pool.run([&trees, &database, &indexes, &mutex, &paths, &max_p](typename c_map_t::const_iterator cat) {
        MVPTree tree = MVPTree::Build(database, paths, max_p, indexes, cat->second.first, cat->second.second + 1);
        std::lock_guard<std::mutex>* guard = new std::lock_guard<std::mutex>(mutex);
        trees[cat->first] = tree;
        delete guard;
    }, database.C_map);
    assert (trees.size() == database.C_map.size());
    #else
    for (auto cat : database.C_map) {
        trees[cat.first] = MVPTree::Build(database, paths, max_p, indexes, cat.second.first, cat.second.second + 1);
    }
    #endif

    #ifdef CHECK_MVP_FOREST
    for (auto cat : database.C_map) {
        MVPTree::Check(trees[cat.first]);
    }
    #endif

    return {
        .indexes = indexes,
        .paths = paths,
        .trees = trees,
        .max_p = max_p
    };
}

void MVPForest::Search(const MVPForest& forest, const Database& database, score_t* PATH, Result& result, const Query& query) {
  Scoreboard gboard;

  #ifdef DISATTEND_CHECKS
  const uint32_t query_type = NORMAL;
  #else
  const uint32_t query_type = (uint32_t) (query.query_type);
  #endif

  if (query_type == BY_C) {
    forest.trees.at(query.v).knn_search(query, gboard, PATH);
  } else if (query_type == BY_C_AND_T) {
    forest.trees.at(query.v).knn_search(query, gboard, PATH);
  } else if (query_type == BY_T) {
    for (auto tree : forest.trees) {
      tree.second.knn_search(query, gboard, PATH);
    }
  } else {
    for (auto tree : forest.trees) {
      tree.second.knn_search(query, gboard, PATH);
    }
  }

  assert (gboard.full());
  uint32_t rank = gboard.size() - 1;
  while(!gboard.empty()) {
    result.data[rank] = gboard.top().index;
    gboard.pop();
    rank--;
  }
}

void MVPForest::Free(MVPForest& forest) {
    for (auto tree : forest.trees)
        MVPTree::Free(tree.second);
    forest.trees = {};
    if (forest.indexes != nullptr) {
      free(forest.indexes);
      forest.indexes = nullptr;
    }
    if (forest.paths != nullptr) {
      free(forest.paths);
      forest.paths = nullptr;
    }
}
