#include <sigmod/random.hh>
#include <cstdlib>

uint32_t RandomUINT32T(const uint32_t min, const uint32_t max) {
    const uint32_t width = max - min;
    return (std::rand() % width) + min;
}

float32_t RandomFLOAT32T(const float32_t min, const float32_t max) {
    const float32_t width = RAND_MAX / (max - min);
    return (static_cast<float32_t>(std::rand()) / width) + min;
}

void RandomizeRecord(Record& node) {
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        node.fields[i] = ((float)std::rand()) / RAND_MAX;
    }
}

void RandomizeQuery(Query& node) {
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        node.fields[i] = ((float)std::rand()) / RAND_MAX;
    }
}

void RandomizeDatabase(Database& database) {
    for (uint32_t i = 0; i < database.length; i++) {
        RandomizeRecord(database.records[i]);
    }
}

void RandomizeQuerySet(QuerySet& queryset) {
    for (uint32_t i = 0; i < queryset.length; i++) {
        RandomizeQuery(queryset.queries[i]);
    }
}