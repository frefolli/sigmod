#ifndef CONFIG_HH
#define CONFIG_HH

#include <cstdint>

typedef float float32_t;
// this represents the distance between vectors or Recall score
typedef double score_t;

/* Parameters of the problem */
const uint32_t k_nearest_neighbors = 100;
const uint32_t vector_num_dimension = 100;

/* This is eql to vector_num_dimension at startup */
extern uint32_t actual_vector_size;

#endif
