#include <algorithm>
#include <iterator>
#include <ostream>
#include <sigmod/lsh.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/memory.hh>
#include <sigmod/seek.hh>
#include <random>
#include <fstream>
#include <omp.h>
#include <string>
#include <filesystem>

void Chain::build(uint32_t database_length) {
    this->width = LSH_WIDTH(database_length);
    this->k = LSH_K(this->width);
    this->chain = smalloc<Atom>(k);

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_real_distribution<float32_t> uniform(-1, 1);
    
    for (uint32_t i = 0; i < this->k; i++) {
        for (uint32_t j = 0; j < actual_vector_size; j++) {
            this->chain[i].a[j] = uniform(generator);
        }
    }
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
  this->chain.build(length);

  this->hashes = smalloc<hash_t>(this->length);
  #pragma omp parallel for
  for (uint32_t i = 0; i < this->length; i++) {
      this->hashes[i] = chain.hash(database.records[start + i]);
  }

  this->buckets = new std::unordered_map<hash_t, std::vector<uint32_t>>();
  for (uint32_t i = 0; i < this->length; i++) {
      this->buckets->operator[](this->hashes[i]).push_back(start + i);
  }

  #pragma omp parallel for
  for (uint32_t i = 0; i < this->buckets->size(); i++) {
    auto it = this->buckets->begin();
    std::advance(it, i);
    std::sort(it->second.begin(), it->second.end(), [&database](uint32_t a, uint32_t b) {
      if (database.records[a].T != database.records[b].T)
          return (database.records[a].T < database.records[b].T);
      for (uint32_t i = 0; i < actual_vector_size; i++) {
          if (database.records[a].fields[i] != database.records[b].fields[i])
              return (database.records[a].fields[i] < database.records[b].fields[i]);
      }
      return true;
    });
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

void HashTable::dump(const std::string outfile) const {
    std::ofstream out(outfile);
    out << "ID,Count" << std::endl;
    for (auto it = this->buckets->begin(); it != this->buckets->end(); it++) {
        out << it->first << "," << it->second.size() << std::endl;
    }
    out.close();
}

void LSH::search(const Database& database, const Query& query,
                 Scoreboard& board, const uint32_t query_type) const {
    switch(query_type) {
        case BY_T:
        case BY_C_AND_T:
            for (uint32_t i = 0; i < this->N; i++) {
                hash_t hash = this->hashtables[i].chain.hash(query);
		auto it = this->hashtables[i].buckets->find(hash);
		if (it != this->hashtables[i].buckets->end()) {
            std::vector<uint32_t>& vec = it->second;
			uint32_t start = 0;
			uint32_t end = vec.size();
		
			start = SeekHigh(
			    [&database, &vec](uint32_t i) {
			      return database.at(vec[i]).T;
			    },
			    start, end, query.l
			);
			end = SeekLow(
			    [&database, &vec](uint32_t i) {
			      return database.at(vec[i]).T;
			    },
			    start, end, query.r
			) + 1;

			for (uint32_t j = start; j < end; j++) {
			    const uint32_t index = vec[j];
			    score_t score = distance(query, database.records[index]);
			    if (elegible_by_T(query, database.records[index]))
				board.pushs(index, score);
			}
		}
            }
            break;
        default:
            for (uint32_t i = 0; i < this->N; i++) {
                hash_t hash = this->hashtables[i].chain.hash(query);
                auto it = this->hashtables[i].buckets->find(hash);
                if (it != this->hashtables[i].buckets->end()) {
                    std::vector<uint32_t>& vec = it->second;
                    for (uint64_t index : vec) {
                        score_t score = distance(query, database.records[index]);
                        board.pushs(index, score);
                    }
                }
            }
            break;
    }
};

void LSH::build(const Database& database, const uint32_t start, const uint32_t end) {
    this->N = LSH_TABLES;
    // std::cout << LSH_TABLES << std::endl;
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

void LSHForest::search(const Database& database, Result& result, const Query& query) const {
    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    Scoreboard board;
    switch (query_type) {
        case BY_C:
        case BY_C_AND_T:
            this->mapped[(uint32_t) query.v].search(database, query, board, query_type);
            break;
        default:
            this->general.search(database, query, board, query_type);
            break;
    }

    uint32_t rank = board.size() - 1;
    while(!board.empty()) {
        #ifdef TRANSLATE_INDEXES
        result.data[rank] = database.records[board.top().index].index;
        #else
        result.data[rank] = board.top().index;
        #endif
        board.pop();
        rank -= 1;
    }
};

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
      const uint32_t length = range.second + 1 - range.first;
      if (length >= LSH_FOREST_TRESHOLD) {
          this->mapped[i].build(database, range.first, range.second + 1);
          LogTime("Built Map for C = " + std::to_string(i) + ", len = " + LengthToString(length));
      } else {
        this->mapped[i].hashtables = nullptr;
        this->mapped[i].N = 0;
      }
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

void LSH::dump(const std::string outdir) const {
    if (hashtables != nullptr) {
        std::filesystem::create_directories(outdir);
        for (uint32_t i = 0; i < N; i++) {
            hashtables[i].dump(outdir + "/" + std::to_string(i) + ".csv");
        }
        
        std::ofstream out (outdir + ".csv");
        out << "ID,Width,K,Shift,HC,HMean,HMin,HMax" << std::endl;
        for (uint32_t i = 0; i < N; i++) {
            out << i
                << "," << hashtables[i].chain.width
                << "," << hashtables[i].chain.k
                << std::endl;
        }
        out.close();
    }
}

void LSHForest::dump() const {
    general.dump("lsh_dump/general");
    /*
    for (uint32_t i = 0; i < length_mapped; i++) {
        mapped[i].dump("lsh_dump/" + std::to_string(i));
    }
    */
}
