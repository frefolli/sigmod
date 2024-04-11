#ifndef RANDOM_HH
#define RANDOM_HH

#include <sigmod/config.hh>
#include <sigmod/database.hh>
#include <sigmod/query_set.hh>

uint32_t RandomUINT32T(const uint32_t min, const uint32_t max);
float32_t RandomFLOAT32T(const float32_t min, const float32_t max);

void RandomizeRecord(Record& node);
void RandomizeQuery(Query& node);
void RandomizeDatabase(Database& database);
void RandomizeQuerySet(QuerySet& queryset);

#endif
