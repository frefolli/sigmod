#ifndef EXAUSTIVE_HH
#define EXAUSTIVE_HH

#include <sigmod/database.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>
#include <sigmod/solution.hh>

bool elegible_by_T(const Query& query, const Record& record);

void FilterIndexesByT(const Database& database, uint32_t& start_index, uint32_t& end_index, const float32_t l, const float32_t r);

void FilterIndexesByC(const c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, const float32_t v);

void ExaustiveSearchByT(const Database& database, const Query& query, Scoreboard& scoreboard, const uint32_t start_index, const uint32_t end_index);

void ExaustiveSearch(const Database& database, const Query& query, Scoreboard& scoreboard, const uint32_t start_index, const uint32_t end_index);

void SearchExaustive(const Database& database, const c_map_t& C_map, Result& result, const Query& query);

#endif
