#include <sigmod/database.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>

Database ReadDatabase(const std::string input_path) {
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
        .records = records,
        .C_map = {},
        .indexes = nullptr
    };
}

void FreeDatabase(Database& database) {
    if (database.records == nullptr)
        return;
    free(database.records);
    database.records = nullptr;
    database.length = 0;
}

bool operator<(const Record& a, const Record& b) {
    if (a.C != b.C)
        return (a.C < b.C);
    if (a.T != b.T)
        return (a.T < b.T);
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        if (a.fields[i] != b.fields[i])
            return (a.fields[i] < b.fields[i]);
    }
    return true;
}

void IndexDatabase(Database& database) {
    database.indexes = (uint32_t*) malloc (sizeof(uint32_t) * database.length);
    for (uint32_t i = 0; i < database.length; i++) {
      database.indexes[i] = i;
    }
    std::sort(database.indexes, database.indexes + database.length,
              [&database](uint32_t a, uint32_t b) {
        return database.records[a] < database.records[b];
    });
    float32_t cur_C = database.at(0).C;
    uint32_t cur_start = 0;
    uint32_t cur_end = 0;
    for (uint32_t i = 1; i < database.length; i++) {
        float32_t Ci = database.at(i).C;
        if (Ci == cur_C) {
            cur_end += 1;
        } else {
            database.C_map[cur_C] = {cur_start, cur_end};
            cur_C = Ci;
            cur_start = i;
            cur_end = cur_start;
        }
    }
    database.C_map[cur_C] = {cur_start, cur_end};
}

void StatsDatabase(const Database& database) {
    std::cout << CategoricalEntry::forArrayCellField(
        [&database](uint32_t i) { return database.at(i).C; },
        database.length, "record.C"
    ) << std::endl;

    std::cout << ScalarEntry::forArrayCellField(
        [&database](uint32_t i) { return database.at(i).T; },
        database.length, "record.T"
    ) << std::endl;

    for (uint32_t j = 0; j < actual_vector_size; j++) {
        std::cout << ScalarEntry::forArrayCellField(
            [&database, &j](uint32_t i) { return database.at(i).fields[j]; },
            database.length, "record.field#" + std::to_string(j)
        ) << std::endl;
    }
}
