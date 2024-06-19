#ifndef TREE_UTILS_HH
#define TREE_UTILS_HH
/** @file tree_utils.hh */

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <utility>

uint32_t FindFurthestPoint(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t target);

typedef std::pair<uint32_t, score_t> item_t;
void ReorderByCoupledValue(uint32_t* indexes, score_t* coupled_values, uint32_t length);

uint32_t MaximizeSpread(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end);

#endif
