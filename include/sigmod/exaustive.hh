#ifndef EXAUSTIVE_HH
#define EXAUSTIVE_HH

#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/solution.hh>

bool elegible_by_T(const Query& query, const Record& record);

void FilterIndexesByT(Database& database, uint32_t& start_index, uint32_t& end_index, float32_t l, float32_t r);

void FilterIndexesByC(c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, float32_t v);

void ExaustiveSearchByT(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index);

void ExaustiveSearch(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index);

void SearchExaustive(Result& result, Database& database, c_map_t& C_map, Query& query);

#endif
