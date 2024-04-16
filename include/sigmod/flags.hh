#ifndef FLAGS_HH
#define FLAGS_HH

/* Scoreboard::add always checks if an input Candidate could be already in the collection */
// #define SCOREBOARD_ALWAYS_CHECK_DUPLICATES

/* Query.query_type is ignored as if it was NORMAL */
// #define DISATTEND_CHECKS

/* Compare 2 solutions saved on files */
// #define COMPARE_SOLUTIONS

/* Crafting of solutions and their comparison stops at min(queryset.length, TOT_ELEMENTS) */
#define TOT_ELEMENTS 4
#define STOP_AFTER_TOT_ELEMENTS

/* Apply dimensional reduction before indexing */
// #define ENABLE_DIM_REDUCTION
// #define N_DIM_REDUCTION 70

/* Build a KD Forest and uses it to craft a solution */
// #define ENABLE_KD_FOREST

/* Randomize KD Node dimensions */
// #define KD_FOREST_DIMENSION_RANDOMIZE

/* Assign KD Node dimensions in order to maximize spread */
// #define KD_FOREST_DIMENSION_MAXIMIZE_SPREAD

/* Build a Ball Forest and uses it to craft a solution */
#define ENABLE_BALL_FOREST

/* Build a VP Forest and uses it to craft a solution */
// #define ENABLE_VP_FOREST

/* Build a MVP Forest and uses it to craft a solution */
// #define ENABLE_MVP_FOREST

/* Uses MVPForest::Check on the forest after build */
// #define CHECK_MVP_FOREST

/* Uses the classic linear search to get a solution
 * If used in combo with ENABLE_BALL_FOREST, ENABLE_KD_FOREST ...,
 * it also compares those solutions against this and print a Recall
 * */
// #define ENABLE_EXAUSTIVE

/* Prints mismatch information during such comparisons */
// #define SHOW_MISMATCH_IN_COMPARISON

/* Enables OpenMP */
// #define CONCURRENCY
// #define MAX_CONCURRENCY 2

/* Enables Fast Distance: which map d(a, b) -> d(a, b)^2, thus reducing the amount of operations to be done */
// #define FAST_DISTANCE

/* Enables Fast sqrt: uses Quacke III approximation */
// #define FAST_SQRT

/* Save solution in file */
// #define SAVE_SOLUTION

/* Tracks num of distance() calls */
// #define TRACK_DISTANCE_COMPUTATIONS

/* Use the actual Recall function of the Task */
#define ACCURATE_RECALL

// Enable KMC and Clusterization
#define KMC_EOTI_FS

#endif
