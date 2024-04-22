#include <sigmod/database.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>
#include <cstring>
#include <sigmod/flags.hh>
#include <sigmod/tweaks.hh>
#include <sigmod/dimensional_reduction.hh>
#include <sigmod/lin_alg.hh>
#include <sigmod/memory.hh>

Database ReadDatabase(const std::string input_path) {
    FILE* dbfile = fopen(input_path.c_str(), "rb");
    
    uint32_t db_length;
    fread(&db_length, sizeof(uint32_t), 1, dbfile);

    Record* records = (Record*) std::malloc(sizeof(Record) * db_length);
    RawRecord* records_entry_point = (RawRecord*) records;
    uint32_t records_to_read = db_length;
    while(records_to_read > 0) {
        uint32_t this_batch = BATCH_SIZE;
        if (this_batch > records_to_read) {
            this_batch = records_to_read;
        }
        fread(records_entry_point, sizeof(RawRecord), this_batch, dbfile);
        records_to_read -= this_batch;
        records_entry_point += this_batch;
    }
    fclose(dbfile);

    records_entry_point -= 1;
    for (uint32_t i = db_length - 1; i > 0; i--) {
        std::memmove(records + i, records_entry_point, sizeof(RawRecord));
        records_entry_point -= 1;
        records[i].index = i;
    }
    records[0].index = 0;

    return {
        .length = db_length,
        .records = records,
        .C_map = {},
    };
}

Database CreateDatabaseFromMatrix(const float32_t** db, const uint32_t db_length, const uint32_t vector_dimension){
    Record* records = smalloc<Record>(db_length);
    for (uint32_t i = 0; i < db_length; i++){
        records[i].index = i;
        for (uint32_t j = 0; j < vector_dimension; j++){
            records[i].fields[j] = db[i][j];
            records[i].C = -1;
            records[i].T = -1;
        }
    }
    
    return {
        .length = db_length,
        .records = records,
        .C_map = {}
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
    std::sort(database.records, database.records + database.length,
              [&database](const Record& a, const Record& b) {
        return a < b;
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

    /*
    for (uint32_t j = 0; j < actual_vector_size; j++) {
        std::cout << ScalarEntry::forArrayCellField(
            [&database, &j](uint32_t i) { return database.at(i).fields[j]; },
            database.length, "record.field#" + std::to_string(j)
        ) << std::endl;
    }
    */
}

float32_t** GetFields(
        const Database& database,
        const uint32_t dimension) {
    float32_t** fields = MallocMatrix(database.length, dimension);
    
    for (uint32_t i = 0; i < database.length; i++) {
        Record r = database.records[i];
        fields[i] = r.fields;
    }    

    return fields;
}

void SetFields(
        Database& database,
        float32_t** fields,
        const uint32_t n_component) {

    for (uint32_t i = 0; i < database.length; i++) {
        for (uint32_t j = 0; j < n_component; j++) {
            database.records[i].fields[j] = fields[i][j];
        }
    }    
}

const float32_t** ReduceDimensionality(
        Database& database, 
        const uint32_t final_dimension) {
    const float32_t** prj_matrix = GenerateProjectionMatrix(final_dimension, vector_num_dimension);
    
    RandomProjectionOnDataset(database, vector_num_dimension, final_dimension);

    return prj_matrix;
}
