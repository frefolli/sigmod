#ifndef DATABASE_HH
#define DATABASE_HH

#include <sigmod/record.hh>
#include <sigmod/c_map.hh>
#include <string>

struct Database {
    uint32_t length;
    Record* records;
};

Database ReadDatabase(std::string input_path);
void FreeDatabase(Database& database);
void StatsDatabase(Database& database);

/* C_map is a mapping of C -> [start, end] positions
 * **note the inclusion of the end**
 * */

void IndexDatabase(Database& database,
                   c_map_t& C_map);

#endif
