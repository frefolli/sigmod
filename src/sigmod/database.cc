#include <sigmod/database.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <map>

Database ReadDatabase(std::string input_path) {
    FILE* dbfile = fopen(input_path.c_str(), "rb");
    
    uint32_t db_length;
    fread(&db_length, sizeof(uint32_t), 1, dbfile);

    Record* records = (Record*) std::malloc(sizeof(Record) * db_length);
    Record* records_entry_point = records;
    uint32_t records_to_read = db_length;
    while(records_to_read > 0) {
        uint32_t this_batch = batch_size;
        if (this_batch > records_to_read) {
            this_batch = records_to_read;
        }
        fread(records_entry_point, sizeof(Record), this_batch, dbfile);
        records_to_read -= this_batch;
        records_entry_point += this_batch;
    }
    fclose(dbfile);

    return {
        .length = db_length,
        .records = records
    };
}

void FreeDatabase(Database& database) {
    if (database.records == nullptr)
        return;
    free(database.records);
    database.records = nullptr;
    database.length = 0;
}

bool operator<(Record& a, Record& b) {
    if (a.C != b.C)
        return (a.C < b.C);
    if (a.T != b.T)
        return (a.T < b.T);
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        if (a.fields[i] != b.fields[i])
            return (a.fields[i] < b.fields[i]);
    }
    return true;
}

void IndexDatabase(Database& database) {
    std::sort(database.records, database.records + database.length);
    
    std::map<float, std::pair<uint32_t, uint32_t>> C_map = {};

    float cur_C = database.records[0].C;
    uint32_t cur_start = 0;
    uint32_t cur_end = 0;
    for (uint32_t i = 1; i < database.length; i++) {
        if (database.records[i].C == cur_C) {
            cur_end += 1;
        } else {
            C_map[cur_C] = {cur_start, cur_end};
            cur_C = database.records[i].C;
            cur_start = i;
            cur_end = cur_start;
        }
    }
    C_map[cur_C] = {cur_start, cur_end};
}

void StatsDatabase(Database& database) {
    std::cout << CategoricalEntry::forArrayCellField(
        [&database](uint32_t i) { return database.records[i].C; },
        database.length, "record.C"
    ) << std::endl;

    std::cout << ScalarEntry::forArrayCellField(
        [&database](uint32_t i) { return database.records[i].T; },
        database.length, "record.T"
    ) << std::endl;

    for (uint32_t j = 0; j < vector_num_dimension; j++) {
        std::cout << ScalarEntry::forArrayCellField(
            [&database, &j](uint32_t i) { return database.records[i].fields[j]; },
            database.length, "record.field#" + std::to_string(j)
        ) << std::endl;
    }
}
