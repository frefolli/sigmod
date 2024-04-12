#ifndef DATABASE_HH
#define DATABASE_HH

#include <sigmod/record.hh>
#include <sigmod/c_map.hh>
#include <string>
#include <sigmod/flags.hh>

struct Database {
    uint32_t length;
    Record* records;
    c_map_t C_map;

    // indirection of indexes[index]
    inline const Record& at(const uint32_t index) const {
      return records[index];
    };
};

Database ReadDatabase(const std::string input_path);
void FreeDatabase(Database& database);
void StatsDatabase(const Database& database);

void IndexDatabase(Database& database);
void ClusterizeDatabase(const Database& database);

const float32_t** ReduceDimensionality(Database& database, const uint32_t final_dimension);

#endif
