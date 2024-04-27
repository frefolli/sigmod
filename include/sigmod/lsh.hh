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
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t k;
    Atom* chain;

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

    void build(uint32_t database_length);
    static void Free(Chain& chain);
};

struct HashTable {
    Chain chain;
    hash_t* hashes;
    uint32_t length;
    std::unordered_map<hash_t, std::vector<uint32_t>>* buckets;
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
