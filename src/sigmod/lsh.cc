#include <sigmod/lsh.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/memory.hh>
#include <cmath>
#include <random>
#include <fstream>
#include <omp.h>
#include <string>

#define LSH_SPREAD 25
#define LSH_DEPTH 1
#define LSH_TABLES 100
#define LSH_WIDTH(length) std::sqrt(length) * std::log10(length) / 2

void Chain::build(uint32_t database_length) {
    this->width = LSH_WIDTH(database_length);
    this->k = LSH_DEPTH;
    this->chain = smalloc<Atom>(k);

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_real_distribution<float32_t> uniform(0, width);
    std::normal_distribution<float32_t> normal(0, LSH_SPREAD);
    
    for (uint32_t i = 0; i < this->k; i++) {
        for (uint32_t j = 0; j < actual_vector_size; j++) {
            this->chain[i].a[j] = normal(generator);
        }
        this->chain[i].b = uniform(generator);
    }
}

hash_t Chain::hash(Record& record) const {
    hash_t _hash = 0;
    for (uint32_t i = 0; i < k; i++) {
        score_t sum = 0;
        for (uint32_t j = 0; j < actual_vector_size; j++) {
            sum += chain[i].a[j] * record.fields[j];
        }
        uint32_t h = ((uint32_t) std::floor(sum + chain[i].b)) % width;
        _hash = ((_hash << 3) + h) % width;
    }
    return _hash;
}

void Chain::Free(Chain& chain) {
    if (chain.chain != nullptr) {
        free(chain.chain);
        chain.chain = nullptr;
        chain.k = 0;
        chain.width = 0;
    }
}

void HashTable::build(const Database& database, const uint32_t start, const uint32_t end) {
    this->length = end - start;
    this->chain.build(database.length);

    this->hashes = smalloc<hash_t>(this->length);
    #pragma omp parallel for
    for (uint32_t i = 0; i < this->length; i++) {
        this->hashes[i] = chain.hash(database.records[start + i]);
    }

    this->buckets = new std::map<hash_t, std::vector<uint32_t>>();
    this->max_hash = 0;
    for (uint32_t i = 0; i < this->length; i++) {
        this->buckets->operator[](this->hashes[i]).push_back(start + i);
        if (this->hashes[i] > this->max_hash)
            this->max_hash = this->hashes[i];
    }
}

void HashTable::Free(HashTable& hashtable) {
    if (hashtable.hashes != nullptr) {
        free(hashtable.hashes);
        hashtable.hashes = nullptr;
        hashtable.length = 0;
        delete hashtable.buckets;
        hashtable.buckets = nullptr;
        Chain::Free(hashtable.chain);
    }
}

void HashTable::dump(const std::string outfile) {
    std::ofstream out(outfile);
    out << "ID,Count" << std::endl;
    for (auto it = this->buckets->begin(); it != this->buckets->end(); it++) {
        out << it->first << "," << it->second.size() << std::endl;
    }
    out.close();
}

void LSH::lshSearch(const Database& database, const uint32_t target_index) {
    Record& target = database.records[target_index];
    Scoreboard board;
    for (uint32_t i = 0; i < N; i++) {
        hash_t hash = this->hashtables[i].chain.hash(target);
        for (uint64_t index : this->hashtables[i].buckets->at(hash)) {
            score_t score = distance(target, database.records[index]);
            board.pushs(index, score);
        }
    }

    std::ofstream out("lshsq_" + std::to_string(target_index) + ".csv");
    out << "Rank,ID,Score" << std::endl;
    uint32_t rank = board.size() - 1;
    while(!board.empty()) {
        out << rank << "," << board.top().index << "," << board.top().score << std::endl;
        board.pop();
        rank--;
    }
    out.close();
}

void LSH::eSearch(const Database& database, const uint32_t target_index) {
    Record& target = database.records[target_index];
    Scoreboard board;
    for (uint32_t index = 0; index < database.length; index++) {
        score_t score = distance(target, database.records[index]);
        board.pushs(index, score);
    }

    std::ofstream out("esq_" + std::to_string(target_index) + ".csv");
    out << "Rank,ID,Score" << std::endl;
    uint32_t rank = board.size() - 1;
    while(!board.empty()) {
        out << rank << "," << board.top().index << "," << board.top().score << std::endl;
        board.pop();
        rank--;
    }
    out.close();
}

void LSH::build(const Database& database, const uint32_t start, const uint32_t end) {
    this->N = LSH_TABLES;
    this->hashtables = smalloc<HashTable>(this->N);
    
    for (uint32_t i = 0; i < N; i++) {
        hashtables[i].build(database, start, end);
    }
}

void LSH::Free(LSH& lsh) {
    if (lsh.hashtables != nullptr) {
        for (uint32_t i = 0; i < lsh.N; i++) {
            HashTable::Free(lsh.hashtables[i]);
        }
        free(lsh.hashtables);
        lsh.hashtables = nullptr;
        lsh.N = 0;
    }
}

void LSHForest::build(const Database& database) {
  this->general.build(database, 0, database.length);
  LogTime("Built LSH Forest :: General");

  uint32_t max_mapped = 0;
  for (auto it : database.C_map) {
    if (it.first > max_mapped) {
      max_mapped = it.first;
    }
  }

  this->length_mapped = max_mapped + 1;
  this->mapped = smalloc<LSH>(this->length_mapped);

  for (uint32_t i = 0; i < this->length_mapped; i++) {
    if (database.C_map.find(i) != database.C_map.end()) {
      auto range = database.C_map.at(i);
      this->mapped[i].build(database, range.first, range.second + 1);
      LogTime("Built Map for C = " + std::to_string(i));
    } else {
      this->mapped[i].hashtables = nullptr;
      this->mapped[i].N = 0;
    }
  }
  LogTime("Built LSH Forest :: Mapped");
}

void LSHForest::Free(LSHForest& forest) {
  if (forest.mapped != nullptr) {
    LSH::Free(forest.general);
    for (uint32_t i = 0; i < forest.length_mapped; i++) {
      LSH::Free(forest.mapped[i]);
    }
    free(forest.mapped);
    forest.mapped = nullptr;
    forest.length_mapped = 0;
  }
}

void ClusterizeDatabase(const Database& database) {
  LSHForest forest;
  forest.build(database);
  LSHForest::Free(forest);
  /*
  for (uint32_t i = 0; i < lsh.N; i++) {
      lsh.hashtables[i].dump("H" + std::to_string(i) + ".csv");
  }
  LogTime("Dumped HTs");

  lsh.lshSearch(database, 7017);
  LogTime("Searched with LSH");

  lsh.eSearch(database, 7017);
  LogTime("Searched with Exaustive");
  
  LogTime("Freed HTs");
  */
}
