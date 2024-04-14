#include <sigmod/lsh.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/memory.hh>
#include <cmath>
#include <omp.h>
#include <random>
#include <fstream>

struct Atom {
    float32_t b;
    float32_t a[vector_num_dimension];
};

struct Chain {
    uint32_t width;
    uint32_t k;
    Atom* chain;

    static Chain Build(uint32_t database_length) {
        const uint32_t k = 5;
        Atom* chain = smalloc<Atom>(k);

        const uint32_t width = std::sqrt(database_length);
        std::random_device random_device;  // a seed source for the random number engine
        std::mt19937 generator(random_device()); // mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<float32_t> uniform(0, width);
        std::normal_distribution<float32_t> normal(0, 1);
        
        for (uint32_t i = 0; i < k; i++) {
            chain[i].b = uniform(generator);
            for (uint32_t j = 0; j < actual_vector_size; j++) {
                chain[i].a[j] = normal(generator);
            }
        }

        return {
            .width = width,
            .k = k,
            .chain = chain
        };
    }

    uint32_t hash(Record& record) {
        uint32_t _hash = 0;
        for (uint32_t i = 0; i < k; i++) {
            score_t sum = 0;
            for (uint32_t j = 0; j < actual_vector_size; j++) {
                sum += chain[i].a[j] * record.fields[j];
            }
            uint32_t h = ((uint32_t) std::floor(sum + chain[i].b)) % width;
            _hash = (_hash << 1) + h;
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
    uint32_t* hashes;
    uint32_t length;

    static HashTable Build(const Database& database) {
        Chain chain = Chain::Build(database.length);

        uint32_t* hashes = smalloc<uint32_t>(database.length);
        for (uint32_t i = 0; i < database.length; i++) {
            hashes[i] = chain.hash(database.records[i]);
        }

        return {
            .chain = chain,
            .hashes = hashes,
            .length = database.length
        };
    }

    static void Free(HashTable& hashtable) {
        if (hashtable.hashes != nullptr) {
            free(hashtable.hashes);
            hashtable.hashes = nullptr;
            hashtable.length = 0;
            Chain::Free(hashtable.chain);
        }
    }

    static void Dump(HashTable& hashtable, std::string outfile) {
        std::map<uint32_t, uint32_t> hashcount = {};

        for (uint32_t i = 0; i < hashtable.length; i++) {
            hashcount[hashtable.hashes[i]] += 1;
        }

        std::ofstream out(outfile);
        out << "ID,Count" << std::endl;
        for (auto it : hashcount) {
            out << it.first << "," << it.second << std::endl;
        }
        out.close();
    }

    static score_t* Similarity(HashTable* hashtables, uint32_t N) {
        score_t* similarity = smalloc<score_t>(N * N);
        for (uint32_t i = 0; i < N; i++) {
            similarity[i * N + i] = 1.0;
            for (uint32_t j = i + 1; j < N; j++) {
                score_t sum = 0.0;
                for (uint32_t k = 0; k < hashtables[i].length; k++) {
                    if (hashtables[i].hashes[k] == hashtables[j].hashes[k]) {
                        sum += 1.0;
                    }
                }
                sum /= hashtables[i].length;
                similarity[i * N + j] = sum;
                similarity[j * N + i] = sum;
            }
        }
        return similarity;
    }

    static void DumpSimilarity(HashTable* hashtables, uint32_t N, std::string outfile) {
        score_t* similarity = Similarity(hashtables, N);

        std::ofstream out(outfile);
        out << "import numpy as np" << std::endl;
        out << "def data():" << std::endl;
        out << "\treturn np.array([" << std::endl; 
        for (uint32_t i = 0; i < N; i++) {
            if (i != 0)
                out << ",\n";
            out << "\t\t[";
            for (uint32_t j = 0; j < N; j++) {
                if (j != 0)
                    out << ",";
                out << " " << similarity[i * N + j];
            }
            out << "]";
        }
        out << "])" << std::endl;
        out.close();

        free(similarity);
    }
};

void ClusterizeDatabase(const Database& database) {
    const uint32_t N = 10;
    HashTable* hashtables = smalloc<HashTable>(N);

    for (uint32_t i = 0; i < N; i++) {
        hashtables[i] = HashTable::Build(database);
    }

    for (uint32_t i = 0; i < N; i++) {
        HashTable::Dump(hashtables[i], "H" + std::to_string(i) + ".csv");
    }

    HashTable::DumpSimilarity(hashtables, N, "similarity.py");
    
    for (uint32_t i = 0; i < N; i++) {
        HashTable::Free(hashtables[i]);
    }
}
