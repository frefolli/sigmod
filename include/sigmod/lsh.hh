#ifndef LSH_HH
#define LSH_HH

#include <sigmod/database.hh>
#include <vector>

typedef uint64_t hash_t;

struct Atom {
    float32_t b;
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t k;
    Atom* chain;

    hash_t hash(Record& record) const;
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
    void dump(const std::string outfile);
    void evaluate(const Database& database, const std::string outfile);
};

struct LSH {
    uint32_t N;
    HashTable* hashtables;
    uint32_t start;
    uint32_t end;

    void build(const Database& database, const uint32_t start, const uint32_t end);
    static void Free(LSH& lsh);

    void lshSearch(const Database& database, const uint32_t target_index);
    void eSearch(const Database& database, const uint32_t target_index);
};

struct LSHForest {
    LSH general;
    LSH* mapped;
    uint32_t length_mapped;

    void build(const Database& database);
    static void Free(LSHForest& forest);
};

void ClusterizeDatabase(const Database& database);

#endif
