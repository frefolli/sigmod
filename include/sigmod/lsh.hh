#ifndef LSH_HH
#define LSH_HH

#include <sigmod/database.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/query.hh>
#include <sigmod/solution.hh>
#include <sigmod/flags.hh>
#include <iostream>
#include <vector>

typedef uint64_t hash_t;

inline hash_t CraftVariant(const hash_t original, const uint16_t pivot) {
    return original ^ ((uint32_t) 1 << pivot);
}

struct Atom {
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t n_of_hash_functions;
    uint32_t start_of_pq;
    uint32_t length_of_pq;
    Atom* chain;

    template<typename WF>
    hash_t hash(const WF& record) {
        hash_t _hash = 0;
        for (uint32_t i = 0; i < n_of_hash_functions; i++) {
            score_t sum = 0;
            for (uint32_t k = 0; k < length_of_pq; k++) {
                const uint32_t j = start_of_pq + k;
                // if you want random start_of_pq, add mod operation above
                sum += chain[i].a[j] * record.fields[j];
            }
            _hash <<= 1; // lshift "in-place", doesn't change if zero
            if (sum >= 0) {
                _hash += 1; // last bit to one, zero otherwise
            }
        }
        return _hash;
    }

    void build(uint32_t database_length, uint32_t ith);
    static void Free(Chain& chain);
};

struct HashTable {
    Chain chain;
    hash_t* hashes;
    uint32_t length;
    std::unordered_map<hash_t, std::vector<uint32_t>>* buckets;
    uint32_t start;
    uint32_t end;

    void build(const Database& database, uint32_t ith, const uint32_t start, const uint32_t end);
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
