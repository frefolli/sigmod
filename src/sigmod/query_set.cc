#include <sigmod/query_set.hh>
#include <cstdio>
#include <iostream>
#include <map>
#include <algorithm>

QuerySet ReadQuerySet(std::string input_path) {
    FILE* dbfile = fopen(input_path.c_str(), "rb");
    
    uint32_t db_length;
    fread(&db_length, sizeof(uint32_t), 1, dbfile);

    Query* queries = (Query*) std::malloc(sizeof(Query) * db_length);
    Query* queries_entry_point = queries;
    uint32_t queries_to_read = db_length;
    while(queries_to_read > 0) {
        uint32_t this_batch = batch_size;
        if (this_batch > queries_to_read) {
            this_batch = queries_to_read;
        }
        fread(queries_entry_point, sizeof(Query), this_batch, dbfile);
        queries_to_read -= this_batch;
        queries_entry_point += this_batch;
    }
    fclose(dbfile);

    return {
        .length = db_length,
        .queries = queries
    };
}

void FreeQuerySet(QuerySet& queryset) {
    if (queryset.queries == nullptr)
        return;
    free(queryset.queries);
    queryset.queries = nullptr;
    queryset.length = 0;
}

bool operator<(Query& a, Query& b) {
    if (a.query_type != b.query_type)
        return (a.query_type < b.query_type);
    if (a.v != b.v)
        return (a.v < b.v);
    if (a.l != b.l)
        return (a.l < b.l);
    if (a.r != b.r)
        return (a.r < b.r);
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        if (a.fields[i] != b.fields[i])
            return (a.fields[i] < b.fields[i]);
    }
    return true;
}

void StatsQuerySet(QuerySet& query_set) {
    std::sort(query_set.queries, query_set.queries + query_set.length);
    
    std::map<float, std::pair<uint32_t, uint32_t>> query_type_map = {};

    float cur_query_type = query_set.queries[0].query_type;
    uint32_t cur_start = 0;
    uint32_t cur_end = 0;
    for (uint32_t i = 1; i < query_set.length; i++) {
        if (query_set.queries[i].query_type == cur_query_type) {
            cur_end += 1;
        } else {
            query_type_map[cur_query_type] = {cur_start, cur_end};
            cur_query_type = query_set.queries[i].query_type;
            cur_start = i;
            cur_end = cur_start;
        }
    }
    query_type_map[cur_query_type] = {cur_start, cur_end};

    std::cout << "query_type,#,start,end" << std::endl;
    for (auto it : query_type_map) {
        std::cout << it.first << "," << (it.second.second-it.second.first+1) << "," << it.second.first << "," << it.second.second << std::endl;
    }
}
