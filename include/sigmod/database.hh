#ifndef DATABASE_HH
#define DATABASE_HH

#include <sigmod/record.hh>
#include <string>

struct Database {
    uint32_t length;
    Record* records;
};

Database ReadDatabase(std::string input_path);
void FreeDatabase(Database& database);
void StatsDatabase(Database& database);
void IndexDatabase(Database& database);

#endif
