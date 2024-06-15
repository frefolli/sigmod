#ifndef DATABASE_HH
#define DATABASE_HH
/** @file database.hh */

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
void CreateDatabaseFromMatrix(Database& database, const float32_t** db, const uint32_t db_length, const uint32_t vector_dimension);
void FreeDatabase(Database& database);
void StatsDatabase(const Database& database);

void IndexDatabase(Database& database);

const float32_t** ReduceDimensionality(Database& database, const uint32_t final_dimension);

#endif
