#ifndef MVP_TREE_HH
#define MVP_TREE_HH

// actual dependencies
#include <sigmod/config.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/solution.hh>
#include <sigmod/query.hh>
#include <sigmod/memory.hh>
#include <sigmod/tree_utils.hh>

#define READ_DATABASE_FROM_FILE
#define READ_QUERYSET_FROM_FILE
#define CHECK_MVP_TREE
#define TRACK_MEMORY_ALLOCATION
#define TRACK_MAX_LEVEL_REACHED
#define TRACK_MALFORMED_LEAFS
#define TRACK_INTERNAL_BRANCH_USAGE

struct MVPNode {
  uint32_t start;
  uint32_t end;
  uint32_t Sv1;
  uint32_t Sv2;
  score_t* D1;
  score_t* D2;
  float32_t M1;
  float32_t M2;
  float32_t M3;
  MVPNode* C1;
  MVPNode* C2;
  MVPNode* C3;
  MVPNode* C4;

  enum {LEAF, INTERNAL} type;

  static MVPNode* NewLeaf(uint32_t start, uint32_t end,
                          uint32_t Sv1, uint32_t Sv2,
                          score_t* D1, score_t* D2);

  static MVPNode* NewInternal(uint32_t start, uint32_t end, uint32_t Sv1, uint32_t Sv2,
                              float32_t M1, float32_t M2, float32_t M3,
                              MVPNode* C1, MVPNode* C2, MVPNode* C3, MVPNode* C4);
  static void Free(MVPNode* node);
};

struct MVPTree {
  Record* records;
  uint32_t length;
  uint32_t* indexes;
  MVPNode* root;

  /* Si[] is memorized continuosly like:
   * paths := {
   *    S0[0], ..., S0[P-1],
   *    S1[0], ..., S1[P-1],
   *    ...,
   *    SN-1[0], ..., SN-1[P-1]
   * }
   * */
  score_t* paths;
  uint32_t k;
  uint32_t p;

  inline uint32_t& at(const uint32_t i) {
    return this->indexes[i];
  }

  MVPNode* build_leaf(uint32_t start, uint32_t end, uint32_t length);
  MVPNode* build_internal(uint32_t start, uint32_t end, uint32_t length, uint32_t level);
  MVPNode* build_node(uint32_t start, uint32_t end, uint32_t level);
  void build(Record* records, uint32_t length, uint32_t* indexes);

  void knn_search_leaf(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node);
  void knn_search_internal(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node);
  void knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH, float32_t& r, uint32_t level, const MVPNode* node);
  void knn_search(const Query& q, Scoreboard& scoreboard, score_t* PATH);

  static uint32_t OptimalK(uint32_t n_of_nearest_neighbors);
  static uint32_t OptimalP(uint32_t n_of_records);

  static void Check(MVPTree& tree, MVPNode* node);
  static void Check(MVPTree& tree);

  static MVPTree New();
  static void Free(MVPTree& tree);
  static MVPTree Build(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);
};

struct MVPForest {
  uint32_t* indexes;
  std::map<uint32_t, MVPTree> trees;

  static MVPForest Build(const Database& database);
  void knn_search(const Database& database, Result& result, const Query& query);
};

#endif