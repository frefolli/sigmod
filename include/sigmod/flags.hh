#ifndef FLAGS_HH
#define FLAGS_HH

/* Scoreboard::add always checks if an input Candidate could be already in the collection */
// #define SCOREBOARD_ALWAYS_CHECK_DUPLICATES

/* Query.query_type is ignored as if it was NORMAL */
// #define DISATTEND_CHECKS

/* Crafting of solutions and their comparison stops at min(queryset.length, TOT_ELEMENTS) */
// #define TOT_ELEMENTS 1000
// #define STOP_AFTER_TOT_ELEMENTS

/* Build a KD Forest and uses it to craft a solution */
// #define ENABLE_KD_FOREST

/* Randomize KD Node dimensions */
// #define KD_FOREST_DIMENSION_RANDOMIZE

/* Assign KD Node dimensions in order to maximize spread */
// #define KD_FOREST_DIMENSION_MAXIMIZE_SPREAD

/* Build a Ball Forest and uses it to craft a solution */
// #define ENABLE_BALL_FOREST

/* Uses the classic linear search to get a solution
 * If used in combo with ENABLE_BALL_FOREST or ENABLE_KD_FOREST,
 * it also compares those solutions against this and print a Recall
 * */
#define ENABLE_EXAUSTIVE

#endif
