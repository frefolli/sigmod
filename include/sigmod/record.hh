#ifndef RECORD_HH
#define RECORD_HH
/** @file record.hh */

#include <ostream>
#include <sigmod/config.hh>
#include <sigmod/flags.hh>

struct RawRecord {
    float32_t C;
    float32_t T;
    float32_t fields[vector_num_dimension];
};

struct Record {
    float32_t C;
    float32_t T;
    float32_t fields[vector_num_dimension];
    uint32_t index;
};

std::ostream& operator<<(std::ostream& out, const Record& record);

#endif
