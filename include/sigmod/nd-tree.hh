#ifndef ND_TREE_HH
#define ND_TREE_HH
/** @file nd_tree.hh */

#include <sigmod/config.hh>
#include <sigmod/record.hh>
#include <sigmod/query.hh>
#include <sigmod/scoreboard.hh>

const uint32_t N_OF_METRICS = 2;

struct NDNode {
    score_t fields[N_OF_METRICS];
    uint32_t index;

    template <typename WithFields>
    void from(WithFields& with_fields, uint32_t index = -1) {
        this->fields[0] = first_metric(with_fields);
        this->fields[1] = second_metric(with_fields);
        this->index = index;
    }
};

#endif
