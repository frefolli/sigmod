#ifndef LSH_HH
#define LSH_HH

#include <sigmod/database.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>
#include <sigmod/flags.hh>
#include <vector>

typedef uint64_t hash_t;

struct Atom {
    float32_t b;
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t shift;
    uint32_t k;
    Atom* chain;

    #ifdef LSH_TRACKING
    uint32_t hash_mean_counter;
    score_t hash_mean_register;
    score_t hash_min_register;
    score_t hash_max_register;
    #endif

    #ifndef NEW_LSH_HASH
    template<typename WF>
    hash_t hash(const WF& record) {
        hash_t _hash = 0;
        for (uint32_t i = 0; i < k; i++) {
            score_t sum = 0;
            for (uint32_t j = 0; j < actual_vector_size; j++) {
                sum += chain[i].a[j] * record.fields[j] * LSH_SPREAD;
            }
            #ifdef LSH_TRACKING
            score_t val = sum + chain[i].b;
            if (hash_mean_counter > 0) {
                hash_mean_register = ((hash_mean_register * hash_mean_counter) + val) / (hash_mean_counter + 1);
                hash_mean_counter++;
            } else {
                hash_mean_register = sum + chain[i].b;
                hash_mean_counter = 1;
            }
            if (val < hash_min_register) {
                hash_min_register = val;
            }
            if (val > hash_max_register) {
                hash_max_register = val;
            }
            #endif
            // uint32_t h = ((uint32_t) std::floor(sum + chain[i].b)) % width;
            // uint32_t h = ((sum + chain[i].b) / (score_t) width);
            uint32_t h = sum + chain[i].b;
            _hash = ((_hash << shift) + h) % width;
        }
        return _hash;
    }
    #else
    template<typename WF>
    hash_t hash(const WF& record) {
        hash_t _hash = 0;
        for (uint32_t i = 0; i < k; i++) {
            score_t sum = 0;
            for (uint32_t j = 0; j < actual_vector_size; j++) {
                sum += chain[i].a[j] * record.fields[j];
            }
            _hash <<= 1; // lshift "in-place", doesn't change if zero
            if (sum >= 0) {
                _hash += 1; // last bit to one, zero otherwise
            }
        }
        return _hash;
    }
    #endif

    void build(uint32_t database_length);
    static void Free(Chain& chain);
};

struct HashTable {
    Chain chain;
    hash_t* hashes;
    uint32_t length;
    std::map<hash_t, std::vector<uint32_t>>* buckets;
    uint64_t max_hash;
    uint32_t start;
    uint32_t end;

    void build(const Database& database, const uint32_t start, const uint32_t end);
    static void Free(HashTable& hashtable);
    void dump(const std::string outfile) const;
};

struct LSH {
    uint32_t N;
    HashTable* hashtables;
    uint32_t start;
    uint32_t end;

    void search(const Database& database, const Query& query, Scoreboard& board, const uint32_t query_type) const;
    void build(const Database& database, const uint32_t start, const uint32_t end);
    static void Free(LSH& lsh);
    void dump(const std::string outdir) const;
};

struct LSHForest {
    LSH general;
    LSH* mapped;
    uint32_t length_mapped;

    void search(const Database& database, Result& result, const Query& query) const;
    void build(const Database& database);
    static void Free(LSHForest& forest);
    void dump() const;
};

#endif
