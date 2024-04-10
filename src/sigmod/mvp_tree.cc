#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <queue>
#include <stdexcept>
#include <string>
#include <iostream>
#include <cassert>
#include <map>
#include <cfloat>

// #include <fstream>
// #include <set>
// #include <vector>
// #include <ctime>

#define float32_t float

const uint32_t DIMENSION = 100;
const uint32_t DATABASE_LENGTH = 1000000;
const uint32_t QUERYSET_LENGTH = 1;
const uint32_t KNN = 100;
const uint32_t BATCH_SIZE = 10000;

// #define READ_DATABASE_FROM_FILE
#define CHECK_MVP_TREE
#define TRACK_MEMORY_ALLOCATION
#define TRACK_MAX_LEVEL_REACHED
#define TRACK_MALFORMED_LEAFS
#define TRACK_DISTANCE_COMPUTATIONS
#define TRACK_INTERNAL_BRANCH_USAGE
#define TARGET 1984

auto SIGMOD_LOG_TIME = std::chrono::high_resolution_clock::now();
long long SIGMOD_MEMORY_TRACKER = 0;
uint32_t SIGMOD_MAX_LEVEL_REACHED = 0;
uint32_t SIGMOD_MALFORMED_LEAFS = 0;
uint32_t SIGMOD_DISTANCE_COMPUTATIONS = 0;
std::map<uint32_t, uint32_t> SIGMOD_INTERNAL_BRANCH_USAGE = {};

void LogTime(const std::string s) {
  auto now = std::chrono::high_resolution_clock::now();
  std::cout << "# TIME | " << s << " | elapsed "
            << std::chrono::duration_cast<std::chrono::milliseconds>(now - SIGMOD_LOG_TIME).count()
            << " ms" << std::endl;
  SIGMOD_LOG_TIME = now;
}

std::string BytesToString(long long bytes) {
    std::string rep = "";
    static const long long KILO = 1024;
    static const long long MEGA = KILO*1024;
    static const long long GIGA = MEGA*1024;
    static const long long TERA = GIGA*1024;
    if (bytes > TERA) {
        rep = std::to_string(bytes/TERA) + " TB";
    } else if (bytes > GIGA) {
        rep = std::to_string(bytes/GIGA) + " GB";
    } else if (bytes > MEGA) {
        rep = std::to_string(bytes/MEGA) + " MB";
    } else if (bytes > KILO) {
        rep = std::to_string(bytes/KILO) + " KB";
    } else {
        rep = std::to_string(bytes) + " B";
    }
    return rep;
}

void LogMemory(const std::string s) {
    std::cout << "# MEMORY | " << s << " | Allocated " << BytesToString(SIGMOD_MEMORY_TRACKER) << std::endl;
    SIGMOD_MEMORY_TRACKER = 0;
}

template<typename T>
T* smalloc(uint32_t length = 1, std::string help = "") {
  #ifdef TRACK_MEMORY_ALLOCATION
  SIGMOD_MEMORY_TRACKER += sizeof(T) * length;
  #endif
  T* ptr = (T*) malloc(sizeof(T) * length);
  if (nullptr == ptr) {
    std::string error_message = "memoria finita, non posso allocare " + std::to_string(length) + " bytes";
    if (help.size() != 0)
      error_message += " per " + help;
    throw std::runtime_error(error_message);
  }
  return ptr;
}


struct Scoreboard {
    std::vector<std::pair<uint32_t, float32_t>> data;

    const std::pair<uint32_t, float32_t>& nearest() const {
      return data[0];
    }

    const std::pair<uint32_t, float32_t>& furthest() const {
      return data.back();
    }

    std::pair<uint32_t, float32_t> pop_nearest() {
      std::pair<uint32_t, float32_t> point = data[0];
      data.erase(data.begin());
      return point;
    }

    std::pair<uint32_t, float32_t> pop_furthest() {
      std::pair<uint32_t, float32_t> point = data.back();
      data.pop_back();
      return point;
    }

    void sort() {
      if (data.size() > 1) {
        std::sort(data.begin(), data.end(), [](
          const std::pair<uint32_t, float32_t>& a,
          const std::pair<uint32_t, float32_t>& b
        ) {
          return a.second < b.second;
        });
      }
    }

    bool inline contains(uint32_t index) {
      return std::find_if(data.cbegin(), data.cend(),
                          [&index](const std::pair<uint32_t, float32_t>& other) {
                            return index == other.first;
                          }) != data.cend();
    }

    void add(const std::pair<uint32_t, float32_t>& point) {
      #ifdef ALWAYS_CHECK_DUPLICATES_IN_QUEUE
      if (contains(point.first)) {
          return;
      }
      #endif
      data.push_back(point);
      sort();
    }

    void add(const uint32_t index, const float32_t distance) {
      #ifdef ALWAYS_CHECK_DUPLICATES_IN_QUEUE
      if (contains(index)) {
          return;
      }
      #endif
      data.push_back({index, distance});
      sort();
    }

    void push(const uint32_t index, const float32_t distance) {
      // assumes insertions only if distance < furthest
      if (full()) {
        pop_furthest();
      }
      add(index, distance);
    }

    uint32_t size() const {
      return data.size();
    }

    bool full() const {
      return data.size() == KNN;
    }

    bool not_full() const {
      // return empty();
      return data.size() < KNN;
    }

    bool empty() const {
      return data.size() == 0;
    }
    
    void clear() {
      data.resize(0);
    }
};

struct RawRecord {
    float32_t C;
    float32_t T;
    float32_t fields[DIMENSION];
};

struct Record {
    float32_t C;
    float32_t T;
    float32_t fields[DIMENSION];
    uint32_t index;
};

struct Database {
    uint32_t length;
    Record* records;
};

Database ReadDatabase(const std::string input_path) {
    FILE* dbfile = fopen(input_path.c_str(), "rb");
    
    uint32_t db_length;
    fread(&db_length, sizeof(uint32_t), 1, dbfile);

    Record* records = (Record*) std::malloc(sizeof(Record) * db_length);
    RawRecord* records_entry_point = (RawRecord*) records;
    uint32_t records_to_read = db_length;
    while(records_to_read > 0) {
        uint32_t this_batch = BATCH_SIZE;
        if (this_batch > records_to_read) {
            this_batch = records_to_read;
        }
        fread(records_entry_point, sizeof(RawRecord), this_batch, dbfile);
        records_to_read -= this_batch;
        records_entry_point += this_batch;
    }
    fclose(dbfile);

    records_entry_point -= 1;
    for (uint32_t i = db_length - 1; i > 0; i--) {
        std::memmove(records + i, records_entry_point, sizeof(RawRecord));
        records_entry_point -= 1;
        records[i].index = i;
    }
    records[0].index = 0;

    return {
        .length = db_length,
        .records = records,
    };
}



void RandomizeRecord(Record& node) {
    for (uint32_t i = 0; i < DIMENSION; i++) {
        node.fields[i] = ((float)std::rand()) / RAND_MAX;
    }
}

void RandomizeSet(Record* nodes, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        RandomizeRecord(nodes[i]);
    }
}

float32_t Distance(const Record& a, const Record& b) {
    #ifdef TRACK_DISTANCE_COMPUTATIONS
      SIGMOD_DISTANCE_COMPUTATIONS += 1;
    #endif
    float32_t sum = 0;
    for (uint32_t i = 0; i < DIMENSION; i++) {
        float32_t d = a.fields[i] - b.fields[i];
        sum += d * d;
    }
    return sum;
}

std::pair<uint32_t, float32_t> PickFurthest(const Record* records, uint32_t start, uint32_t end, uint32_t point) {
  uint32_t max_i = start;
  float32_t max_d = Distance(records[point], records[start]);

  for (uint32_t i = start + 1; i < end; i++) {
    float32_t d = Distance(records[point], records[i]);
    if (d > max_d) {
      max_i = i;
      max_d = d;
    }
  }

  return {max_i, max_d};
}

typedef std::pair<uint32_t, float32_t> item_t;
auto compareFunc = [](const item_t& a, const item_t& b) {
  return a.second < b.second;
};

void ReorderByCoupledValue(uint32_t* indexes, float32_t* coupled_values, uint32_t length) {
  std::priority_queue<item_t, std::vector<item_t>, decltype(compareFunc)> items (compareFunc);
  for (uint32_t i = 0; i < length; i++) {
    items.push({indexes[i], coupled_values[i]});
  }
  for (uint32_t i = length; i > 0; i--) {
    indexes[i - 1] = items.top().first;
    coupled_values[i - 1] = items.top().second;
    items.pop();
  }
}

struct MVPNode {
  uint32_t start;
  uint32_t end;
  uint32_t Sv1;
  uint32_t Sv2;
  float32_t* D1;
  float32_t* D2;
  float32_t M1;
  float32_t M2;
  float32_t M3;
  MVPNode* C1;
  MVPNode* C2;
  MVPNode* C3;
  MVPNode* C4;

  enum {LEAF, INTERNAL} type;

  static void Free(MVPNode* node) {
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

  static MVPNode* NewLeaf(uint32_t start, uint32_t end,
                          uint32_t Sv1, uint32_t Sv2,
                          float32_t* D1, float32_t* D2) {
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

  static MVPNode* NewInternal(uint32_t start, uint32_t end,
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
  float32_t* paths;

  uint32_t k;
  uint32_t p;

  inline uint32_t& at(const uint32_t i) {
    return this->indexes[i];
  }

  MVPNode* build_leaf(uint32_t start, uint32_t end, uint32_t length) {
    uint32_t Sv1 = start;
    float32_t* D1 = smalloc<float32_t>(length - 1, "Leaf::D1");
    uint32_t furthest_i = 0;
    float32_t furthest_d = Distance(records[this->at(start + 1)], records[this->at(Sv1)]);
    D1[0] = furthest_d;
    for (uint32_t i = 1; i < length - 1; i++) {
      D1[i] = Distance(records[this->at(start + 1 + i)], records[this->at(Sv1)]);
      if (D1[i] > furthest_d) {
        furthest_d = D1[i];
        furthest_i = i;
      }
    }
    if (furthest_i != 0) {
      std::swap(D1[furthest_i], D1[0]);
      std::swap(this->at(start + 1 + furthest_i), this->at(start + 1));
    }

    float32_t* D2 = nullptr;
    uint32_t Sv2 = -1;
    if (length - 1 > 0) {
      Sv2 = start + 1;
      D2 = smalloc<float32_t>(length - 2, "Leaf::D2");
      for (uint32_t i = 0; i < length - 2; i++) {
        D2[i] = Distance(records[this->at(start + 2 + i)], records[this->at(start + 1)]);
      }
    }
    #ifndef TRACK_MALFORMED_LEAFS
    else {
        SIGMOD_MALFORMED_LEAFS += 1
    }
    #endif

    return MVPNode::NewLeaf(start, end, Sv1, Sv2, D1, D2);
  }

  MVPNode* build_internal(uint32_t start, uint32_t end, uint32_t length, uint32_t level) {
    uint32_t Sv1 = start;
    float32_t* D1 = smalloc<float32_t>(length - 1, "Internal::D1");
    for (uint32_t i = 0; i < length - 1; i++) {
      D1[i] = Distance(records[this->at(start + 1 + i)], records[this->at(Sv1)]);
      if (level < this->p) {
          uint32_t p_index = this->at(start + 1 + i) * this->p;
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

    float32_t* D2 = smalloc<float32_t>(SS1_length, "Internal::D2");
    for (uint32_t i = 0; i < SS1_length; i++) {
      D2[i] = Distance(records[this->at(SS1_start + i)],
                       records[this->at(Sv2)]);
      if (level + 1 < this->p) {
        uint32_t p_index = this->at(SS1_start + i) * this->p;
        this->paths[p_index + level + 1] = D2[i];
      }
    }
    ReorderByCoupledValue(indexes + SS1_start, D2, SS1_length);

    float32_t* D3 = smalloc<float32_t>(SS2_length - 1, "Internal::D3");
    for (uint32_t i = 0; i < SS2_length - 1; i++) {
      D3[i] = Distance(records[this->at(SS2_start + 1 + i)],
                       records[this->at(Sv2)]);
      if (level + 1 < this->p) {
        uint32_t p_index = this->at(SS2_start + 1 + i) * this->p;
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

  MVPNode* build_node(uint32_t start, uint32_t end, uint32_t level) {
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

  void build(Record* records, uint32_t length) {
    this->k = MVPTree::OptimalK(KNN);
    this->p = MVPTree::OptimalP(length);
    this->records = records;
    this->length = length;
    this->indexes = smalloc<uint32_t>(this->length);
    for (uint32_t i = 0; i < this->length; i++)
        this->indexes[i] = i;
    this->paths = smalloc<float32_t>(this->length * this->p);
    for (uint32_t i = 0; i < this->length; i++)
        this->paths[i] = -1;
    this->root = this->build_node(0, this->length, 0);
  }

  void range_search_leaf(const Record& q, const float32_t r, float32_t* PATH, uint32_t level, const MVPNode* node) {
    const float32_t dSv1 = Distance(q, records[at(node->Sv1)]);
    if (dSv1 <= r) {
      std::cout << "accept L::Sv1 := " << at(node->Sv1) << std::endl;
    }

    const float32_t dSv2 = Distance(q, records[at(node->Sv2)]);
    if (dSv2 <= r) {
      std::cout << "accept L::Sv2 := " << at(node->Sv2) << std::endl;
    }

    uint32_t length = node->end - node->start;
    for (uint32_t i = 0; i < length - 2; i++) {
      if (std::fabs(dSv1 - node->D1[i + 1]) <= r) {
          if (std::fabs(dSv2 - node->D2[i]) <= r) {
            bool compute_d = true;
            const uint32_t p_index = at(node->start + 2 + i) * p;
            for (uint32_t j = 0; j < level; j++) {
              if (PATH[j] != -1 && !(std::fabs(PATH[j] - paths[p_index + j]) <= r)) {
                  compute_d = false;
              }
            }
            if (compute_d) {
              const float32_t diq = Distance(q, records[at(node->start + 2 + i)]);
              if (diq <= r) {
                std::cout << "accept leafed := " << at(node->start + 2 + i) << std::endl;
              }
            }
        }
      }
    }
  }

  void range_search_internal(const Record& q, const float32_t r, float32_t* PATH, uint32_t level, const MVPNode* node) {
    const float32_t dSv1 = Distance(q, records[at(node->Sv1)]);
    if (dSv1 <= r) {
      std::cout << "accept I::Sv1 := " << at(node->Sv1) << std::endl;
    }
    if (level < p) {
      PATH[level] = dSv1;
    }

    const float32_t dSv2 = Distance(q, records[at(node->Sv2)]);
    if (dSv2 <= r) {
      std::cout << "accept I::Sv2 := " << at(node->Sv2) << std::endl;
    }
    if (level + 1 < p) {
      PATH[level + 1] = dSv2;
    }

    #ifdef TRACK_INTERNAL_BRANCH_USAGE
      SIGMOD_INTERNAL_BRANCH_USAGE[1] += 1 * (dSv1 - r <= node->M1);
      SIGMOD_INTERNAL_BRANCH_USAGE[2] += 1 * (dSv2 - r <= node->M2);
      SIGMOD_INTERNAL_BRANCH_USAGE[3] += 1 * (dSv2 + r >= node->M2);
      SIGMOD_INTERNAL_BRANCH_USAGE[4] += 1 * (dSv1 + r >= node->M1);
      SIGMOD_INTERNAL_BRANCH_USAGE[5] += 1 * (dSv2 - r <= node->M3);
      SIGMOD_INTERNAL_BRANCH_USAGE[6] += 1 * (dSv2 + r >= node->M3);
    #endif

    if (dSv1 - r <= node->M1) {
      if (dSv2 - r <= node->M2 && node->C1 != nullptr) {
        range_search(q, r, PATH, level + 2, node->C1);
      }
      if (dSv2 + r >= node->M2 && node->C2 != nullptr) {
        range_search(q, r, PATH, level + 2, node->C2);
      }
    }
    if (dSv1 + r >= node->M1) {
      if (dSv2 - r <= node->M3 && node->C3 != nullptr) {
        range_search(q, r, PATH, level + 2, node->C3);
      }
      if (dSv2 + r >= node->M3 && node->C4 != nullptr) {
        range_search(q, r, PATH, level + 2, node->C4);
      }
    }
  }

  void range_search(const Record& q, const float32_t r, float32_t* PATH, uint32_t level, const MVPNode* node) {
    if (node->type == MVPNode::LEAF) {
      range_search_leaf(q, r, PATH, level, node);
    } else {
      range_search_internal(q, r, PATH, level, node);
    }
  }

  void range_search(const Record& q, const float32_t r) {
    float32_t* PATH = smalloc<float32_t>(this->p);
    for (uint32_t i = 0; i < p; i++) {
      PATH[i] = -1;
    }
    range_search(q, r, PATH, 0, root);
    free(PATH);
  }

  void knn_search_leaf(const Record& q, Scoreboard& scoreboard, float32_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
    const uint32_t Sv1 = at(node->Sv1);
    const uint32_t Sv2 = at(node->Sv2);

    const float32_t dSv1 = Distance(q, records[Sv1]);
    const float32_t dSv2 = Distance(q, records[Sv2]);

    if (dSv1 <= r) {
      scoreboard.push(Sv1, dSv1);
      if (scoreboard.full())
        r = scoreboard.furthest().second;
    }
    if (dSv2 <= r) {
      scoreboard.push(Sv2, dSv2);
      if (scoreboard.full())
        r = scoreboard.furthest().second;
    }

    uint32_t length = node->end - node->start;
    for (uint32_t i = 0; i < length - 2; i++) {
      const uint32_t index = at(node->start + 2 + i);
      if (scoreboard.not_full()) {
        const float32_t diq = Distance(q, records[i]);
        scoreboard.push(i, diq);
        if (scoreboard.full())
          r = scoreboard.furthest().second;
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
                const float32_t diq = Distance(q, records[index]);
                if (diq <= r) {
                  scoreboard.push(index, diq);
                  r = scoreboard.furthest().second;
                }
              }
          }
        }
      }
    }
  }

  void knn_search_internal(const Record& q, Scoreboard& scoreboard, float32_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
    const uint32_t Sv1 = at(node->Sv1);
    const uint32_t Sv2 = at(node->Sv2);

    const float32_t dSv1 = Distance(q, records[Sv1]);
    const float32_t dSv2 = Distance(q, records[Sv2]);

    if (dSv1 <= r) {
      scoreboard.push(Sv1, dSv1);
      if (scoreboard.full())
        r = scoreboard.furthest().second;
    }
    if (dSv2 <= r) {
      scoreboard.push(Sv2, dSv2);
      if (scoreboard.full())
        r = scoreboard.furthest().second;
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

  void knn_search(const Record& q, Scoreboard& scoreboard, float32_t* PATH, float32_t& r, uint32_t level, const MVPNode* node) {
    if (node->type == MVPNode::LEAF) {
      knn_search_leaf(q, scoreboard, PATH, r, level, node);
    } else {
      knn_search_internal(q, scoreboard, PATH, r, level, node);
    }
  }

  void knn_search(const Record& q, Scoreboard& scoreboard, float32_t* PATH) {
    float32_t r = FLT_MAX;
    if (scoreboard.full())
      r = scoreboard.furthest().second;
    knn_search(q, scoreboard, PATH, r, 0, root);
  }

  static MVPTree New() {
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

  static uint32_t OptimalK(uint32_t n_of_nearest_neighbors) {
    return n_of_nearest_neighbors;
  }

  static uint32_t OptimalP(uint32_t n_of_records) {
    static const float32_t a = 1.43027086e+00;
    static const float32_t b = 4.44754191e-01;
    static const float32_t c = 1.94581988e+03;
    static const float32_t d = -3.57875462e+00;
    const float32_t y = a * std::log(b * n_of_records + c) + d;
    return std::round(y);
  }

  static void Free(MVPTree& tree) {
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

  static void Check(MVPTree& tree, MVPNode* node) {
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
            float32_t d1 = Distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C1->start + i)]);
            float32_t d2 = Distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C1->start + i)]);
            assert(d1 <= node->M1);
            assert(d2 <= node->M2);
        }
        for (uint32_t i = 0; i < C2_length; i++) {
            float32_t d1 = Distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C2->start + i)]);
            float32_t d2 = Distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C2->start + i)]);
            assert(d1 <= node->M1);
            assert(d2 >= node->M2);
        }
        for (uint32_t i = 0; i < C3_length; i++) {
            float32_t d1 = Distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C3->start + i)]);
            float32_t d2 = Distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C3->start + i)]);
            assert(d1 >= node->M1);
            assert(d2 <= node->M3);
        }
        for (uint32_t i = 0; i < C4_length; i++) {
            float32_t d1 = Distance(tree.records[tree.at(node->Sv1)], tree.records[tree.at(node->C4->start + i)]);
            float32_t d2 = Distance(tree.records[tree.at(node->Sv2)], tree.records[tree.at(node->C4->start + i)]);
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

  static void Check(MVPTree& tree) {
    Check(tree, tree.root);
  }
};

int main(int argc, char** args) {
    // std::srand(std::time(nullptr));

    #ifdef READ_DATABASE_FROM_FILE
    Database dataset = ReadDatabase("dummy-data.bin");
    Record* database = dataset.records;
    uint32_t database_length = dataset.length;
    #else
    uint32_t database_length = DATABASE_LENGTH;
    Record* database = smalloc<Record>(database_length, "database");
    RandomizeSet(database, database_length);
    LogTime("Created DB");
    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Created DB");
    #endif
    #endif

    Record* queryset = smalloc<Record>(QUERYSET_LENGTH, "queryset");
    RandomizeSet(queryset, QUERYSET_LENGTH);
    LogTime("Created QS");

    #ifdef TRACK_MEMORY_ALLOCATION
    LogMemory("Created QS");
    #endif

    std::cout << "Length = " << database_length << std::endl;
    std::cout << "Queries = " << QUERYSET_LENGTH << std::endl;
    std::cout << "Dimension = " << DIMENSION << std::endl;
    std::cout << "KNN = " << KNN << std::endl;
    LogTime("Start");

    MVPTree tree = MVPTree::New();
    tree.build(database, database_length);
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
    float32_t* PATH = smalloc<float32_t>(tree.p);
    for (uint32_t i = 0; i < QUERYSET_LENGTH; i++) {
      scoreboard.clear();
      tree.knn_search(database[i], scoreboard, PATH);
    }
    LogTime("Queried Tree! KNN");
    free(PATH);
    
    #ifdef TRACK_DISTANCE_COMPUTATIONS
    LogTime("DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS));
    LogTime("Mean DC := " + std::to_string(SIGMOD_DISTANCE_COMPUTATIONS / QUERYSET_LENGTH));
    #endif

    for (uint32_t i = 0; i < QUERYSET_LENGTH; i++) {
        scoreboard.clear();
        for (uint32_t j = 0; j < DATABASE_LENGTH; j++) {
            float32_t d = Distance(database[i], database[j]);
            scoreboard.push(j, d);
        }
    }
    LogTime("Queried Exaustive! KNN");

    MVPTree::Free(tree);
    LogTime("Cleared Tree");
  
    free(database);
    LogTime("Cleared DB");

    free(queryset);
    LogTime("Cleared QS");
    
    LogTime("End");
}
