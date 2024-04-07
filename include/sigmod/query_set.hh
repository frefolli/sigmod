#ifndef QUERY_SET_HH
#define QUERY_SET_HH

#include <sigmod/query.hh>
#include <string>

struct QuerySet {
    uint32_t length;
    Query* queries;
};

QuerySet ReadQuerySet(const std::string input_path);
void WriteQuerySet(const QuerySet& query_set, const std::string output_path);
void FreeQuerySet(QuerySet& query_set);
void StatsQuerySet(const QuerySet& query_set);

void ReduceDimensionality(QuerySet& queryset, const float32_t** prj_matrix, const uint32_t final_dimension);

#endif
