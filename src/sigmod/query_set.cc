#include <sigmod/query_set.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <iostream>
#include <sigmod/lin_alg.hh>
#include <sigmod/random_projection.hh>

QuerySet ReadQuerySet(const std::string input_path) {
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

void WriteQuerySet(const QuerySet& query_set, const std::string output_path) {
    FILE* output = fopen(output_path.c_str(), "wb");

    fwrite(&query_set.length, sizeof(uint32_t), 1, output);
    Query* query_set_entry_point = query_set.queries;
    uint32_t query_set_to_write = query_set.length;
    while(query_set_to_write > 0) {
        uint32_t this_batch = batch_size;
        if (this_batch > query_set_to_write) {
            this_batch = query_set_to_write;
        }
        fwrite(query_set_entry_point, sizeof(Query), this_batch, output);
        query_set_to_write -= this_batch;
        query_set_entry_point += this_batch;
    }

    fclose(output);
}

void FreeQuerySet(QuerySet& queryset) {
    if (queryset.queries == nullptr)
        return;
    free(queryset.queries);
    queryset.queries = nullptr;
    queryset.length = 0;
}

void StatsQuerySet(const QuerySet& query_set) {
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

float32_t** GetFields(
        const QuerySet& queryset,
        const uint32_t dimension) {
    float32_t** fields = MallocMatrix(queryset.length, dimension);
    
    for (uint32_t i = 0; i < queryset.length; i++) {
        Query q = queryset.queries[i];
        fields[i] = q.fields;
    }    

    return fields;
}

void SetFields(
        QuerySet& queryset,
        float32_t** fields,
        const uint32_t n_component) {

    for (uint32_t i = 0; i < queryset.length; i++) {
        for (uint32_t j = 0; j < n_component; j++) {
            queryset.queries[i].fields[j] = fields[i][j];
        }
    }    
}

void ReduceDimensionality(QuerySet& queryset, const float32_t** prj_matrix, const uint32_t final_dimension) {
    RandomProjectionGivenProjMatrixOnQuerySet(queryset, vector_num_dimension, prj_matrix, final_dimension);
}