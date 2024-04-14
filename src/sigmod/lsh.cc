#include <sigmod/lsh.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/memory.hh>
#include <cmath>
#include <omp.h>
#include <random>
#include <fstream>

typedef uint64_t hash_t;

struct Atom {
    float32_t b;
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t k;
    Atom* chain;

    static Chain Build(uint32_t database_length) {
        const uint32_t width = std::sqrt(database_length) * std::log10(database_length) / 2;
        const uint32_t k = 1; // (sizeof(hash_t) * 8) / std::log2(width);
        Atom* chain = smalloc<Atom>(k);

        std::random_device random_device;  // a seed source for the random number engine
        std::mt19937 generator(random_device()); // mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<float32_t> uniform(0, width);
        std::normal_distribution<float32_t> normal(0, 8);
        
        for (uint32_t i = 0; i < k; i++) {
            for (uint32_t j = 0; j < actual_vector_size; j++) {
                chain[i].a[j] = normal(generator);
            }
            chain[i].b = uniform(generator);
        }

        return {
            .width = width,
            .k = k,
            .chain = chain
        };
    }

    hash_t hash(Record& record) const {
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

    static void Free(Chain& chain) {
        if (chain.chain != nullptr) {
            free(chain.chain);
            chain.chain = nullptr;
            chain.k = 0;
            chain.width = 0;
        }
    }
};

struct HashTable {
    Chain chain;
    hash_t* hashes;
    uint32_t length;
    std::map<hash_t, std::vector<uint32_t>>* buckets;

    static HashTable Build(const Database& database) {
        Chain chain = Chain::Build(database.length);

        hash_t* hashes = smalloc<hash_t>(database.length);
        for (uint32_t i = 0; i < database.length; i++) {
            hashes[i] = chain.hash(database.records[i]);
        }

        std::map<hash_t, std::vector<uint32_t>>* buckets = new std::map<hash_t, std::vector<uint32_t>>();
        for (uint32_t i = 0; i < database.length; i++) {
            buckets->operator[](hashes[i]).push_back(i);
        }

        return {
            .chain = chain,
            .hashes = hashes,
            .length = database.length,
            .buckets = buckets
        };
    }

    static void Free(HashTable& hashtable) {
        if (hashtable.hashes != nullptr) {
            free(hashtable.hashes);
            hashtable.hashes = nullptr;
            hashtable.length = 0;
            delete hashtable.buckets;
            hashtable.buckets = nullptr;
            Chain::Free(hashtable.chain);
        }
    }

    static void Dump(const HashTable& hashtable, const std::string outfile) {
        std::ofstream out(outfile);
        out << "ID,Count" << std::endl;
        for (auto it : *hashtable.buckets) {
            out << it.first << "," << it.second.size() << std::endl;
        }
        out.close();
    }

    static void Evaluate(const HashTable& hashtable, const Database& database, const std::string outfile) {
        std::ofstream out(outfile);
        out << "ID,MD" << std::endl;
        for (auto it : *hashtable.buckets) {
            const std::vector<uint32_t>& bucket = it.second;
            const uint32_t length = it.second.size();
            score_t sum = 0;
            for (uint32_t i = 0; i < length; i ++) {
                for (uint32_t j = i + 1; j < length; j ++) {
                    sum += 2 * distance(database.records[bucket.at(i)], database.records[bucket.at(j)]);
                }
            }
            sum /= (length * (length - 1));
            out << it.first << "," << sum << std::endl;
        }
        out.close();
    }

    static void LSHSearch(const HashTable* hashtables, const uint32_t N, const Database& database, const uint32_t target_index) {
        Record& target = database.records[target_index];
        Scoreboard board;
        for (uint32_t i = 0; i < N; i++) {
            hash_t hash = hashtables[i].chain.hash(target);
            for (uint64_t index : hashtables[i].buckets->at(hash)) {
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

    static void ESearch(const HashTable* hashtables, const uint32_t N, const Database& database, const uint32_t target_index) {
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
};

void ClusterizeDatabase(const Database& database) {
    const uint32_t N = 100;
    HashTable* hashtables = smalloc<HashTable>(N);

    for (uint32_t i = 0; i < N; i++) {
        hashtables[i] = HashTable::Build(database);
    }
    LogTime("Built HTs");

    for (uint32_t i = 0; i < N; i++) {
        HashTable::Dump(hashtables[i], "H" + std::to_string(i) + ".csv");
    }
    LogTime("Dumped HTs");

    for (uint32_t i = 0; i < N; i++) {
        HashTable::Evaluate(hashtables[i], database, "mH" + std::to_string(i) + ".csv");
    }
    LogTime("Evaluated HTs");

    HashTable::LSHSearch(hashtables, N, database, 7017);
    LogTime("Searched with LSH");
    HashTable::ESearch(hashtables, N, database, 7017);
    LogTime("Searched with Exaustive");
    
    for (uint32_t i = 0; i < N; i++) {
        HashTable::Free(hashtables[i]);
    }
    LogTime("Freed HTs");
}
