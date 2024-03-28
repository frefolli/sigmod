#include <sigmod/query_set.hh>
#include <sigmod/stats.hh>
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

void StatsQuerySet(QuerySet& query_set) {
    std::cout << CategoricalEntry::forArrayCellField(
        [&query_set](uint32_t i) { return query_set.queries[i].query_type; },
        query_set.length, "query.query_type"
    ) << std::endl;

    std::cout << CategoricalEntry::forArrayCellField(
        [&query_set](uint32_t i) { return query_set.queries[i].v; },
        query_set.length, "query.v"
    ) << std::endl;

    std::cout << ScalarEntry::forArrayCellField(
        [&query_set](uint32_t i) { return query_set.queries[i].l; },
        query_set.length, "query.l"
    ) << std::endl;

    std::cout << ScalarEntry::forArrayCellField(
        [&query_set](uint32_t i) { return query_set.queries[i].r; },
        query_set.length, "query.r"
    ) << std::endl;
}
