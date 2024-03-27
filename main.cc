#include <cstdio>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <queue>
#include <vector>

typedef float float32_t;
typedef double score_t;

const uint32_t k_nearest_neighbors = 100;
const uint32_t vector_num_dimension = 100;
const uint32_t batch_size = 10000;

typedef struct {
    float32_t C;
    float32_t T;
    float32_t fields[vector_num_dimension];
} Record;

typedef struct {
    uint32_t length;
    Record* records;
} Database;

typedef struct {
    uint32_t query_type;
    float32_t v;
    float32_t l;
    float32_t r;
    float32_t fields[vector_num_dimension];
} Query;

typedef struct {
    uint32_t length;
    Query* queries;
} QuerySet;

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

void FreeDatabase(Database& database) {
    if (database.records == nullptr)
        return;
    free(database.records);
    database.records = nullptr;
    database.length = 0;
}

void FreeQuerySet(QuerySet& queryset) {
    if (queryset.queries == nullptr)
        return;
    free(queryset.queries);
    queryset.queries = nullptr;
    queryset.length = 0;
}

void PrintRecord(Record& record) {
    std::cout << record.C << "|" << record.T;
    for (uint32_t i = 0; i < vector_num_dimension; i++)
        std::cout << "|" << record.fields[i];
    std::cout << std::endl;
}

void PrintQuery(Query& query) {
    std::cout << query.query_type << "|" << query.v << "|" << query.l << "|" << query.r;
    for (uint32_t i = 0; i < vector_num_dimension; i++)
        std::cout << "|" << query.fields[i];
    std::cout << std::endl;
}

inline bool elegible_by_C(const Query& query, const Record& record) {
    return (query.v == record.C);
}

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

inline bool RecordIsElegible(const Query& query, const Record& record) {
    switch(query.query_type) {
        case 0: {
            return true;
        };
        case 1: {
            return elegible_by_C(query, record);
        };
        case 2: {
            return elegible_by_T(query, record);
        };
        case 3: {
            return (elegible_by_C(query, record) && elegible_by_T(query, record));
        };
    }
    return true;
}

inline score_t distance(const Query& query, const Record& record) {
    score_t sum = 0;
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        sum += pow(query.fields[i] - record.fields[i], 2);
    }
    return sum;
}

typedef struct {
    uint32_t index;
    score_t score;
} Candidate;

const auto compare_function = [](Candidate a, Candidate b) { return a.score < b.score; };
typedef std::priority_queue<Candidate, std::vector<Candidate>, decltype(compare_function)> Scoreboard;

void OutputSolution(FILE* solution, Scoreboard& scoreboard) {
    static uint32_t* buffer = nullptr;

    if (buffer == nullptr)
        buffer = (uint32_t*) malloc(sizeof(uint32_t) * k_nearest_neighbors);

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        buffer[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
    fwrite(solution, sizeof(uint32_t), k_nearest_neighbors, solution);
}

void FindForQuery(FILE* solution, Database& database, Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard(compare_function);

    for (uint32_t i = 0; i < k_nearest_neighbors; i++) {
        scoreboard.emplace(Candidate({
            .index = i,
            .score = distance(query, database.records[i])
        }));
    }

    for (uint32_t i = k_nearest_neighbors; i < database.length; i++) {
        scoreboard.emplace(Candidate({
            .index = i,
            .score = distance(query, database.records[i])
        }));
        scoreboard.pop();
    }

    OutputSolution(solution, scoreboard);
}

void SolveForQueries(std::string output_path, Database& database, QuerySet& query_set) {
    FILE* solution = fopen(output_path.c_str(), "wb");
    for (uint32_t i = 0; i < query_set.length; i++) {
        FindForQuery(solution, database, query_set.queries[i]);
    }
}

int main(int argc, char** args) {
    std::string database_path = "dummy-data.bin";
    std::string query_set_path = "dummy-queries.bin";
    std::string output_path = "output.bin";

    if (argc > 1) {
        database_path = std::string(args[1]);
    }

    if (argc > 2) {
        query_set_path = std::string(args[2]);
    }

    if (argc > 3) {
        output_path = std::string(args[3]);
    }
    
    Database database = ReadDatabase(database_path);
    QuerySet query_set = ReadQuerySet(query_set_path);

    SolveForQueries(output_path, database, query_set);

    FreeDatabase(database);
    FreeQuerySet(query_set);
}
