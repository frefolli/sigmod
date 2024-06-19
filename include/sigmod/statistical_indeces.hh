#ifndef STATISTICAL_INDECES_HH
#define STATISTICAL_INDECES_HH
/** @file statistical_indeces.hh */

#include<sigmod/database.hh>

struct StatisticalIndeces {
    Database* db;
    float32_t* mean;
    float32_t* var;
};

StatisticalIndeces* MallocStatisticalIndeces(Database& database);
void Standardize(StatisticalIndeces& database);
void InitializeStatisticalIndeces(StatisticalIndeces& si);
void ComputeMean(StatisticalIndeces& si);
void ComputeVariance(StatisticalIndeces& si);

void FreeStatisticalIndeces(StatisticalIndeces& si);



#endif
