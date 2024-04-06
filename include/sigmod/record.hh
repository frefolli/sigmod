#ifndef RECORD_HH
#define RECORD_HH

#include <ostream>
#include <sigmod/config.hh>
#include <sigmod/flags.hh>

#ifdef FAST_INDEX
struct RawRecord {
    float32_t C;
    float32_t T;
    float32_t fields[vector_num_dimension];
};
#endif

struct Record {
    float32_t C;
    float32_t T;
    float32_t fields[vector_num_dimension];
    #ifdef FAST_INDEX
    uint32_t index;
    #endif
};

std::ostream& operator<<(std::ostream& out, const Record& record);

#endif
