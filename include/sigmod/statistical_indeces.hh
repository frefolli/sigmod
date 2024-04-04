#ifndef STATISTICAL_INDECES_HH
#define STATISTICAL_INDECES_HH

#include<sigmod/database.hh>

struct StatisticalIndeces {
    Database* db;
    float32_t* mean;
    float32_t* var;
    float32_t* covarianceMatrix;
};

StatisticalIndeces* MallocStatisticalIndeces(Database& database);
void Standardize(StatisticalIndeces& database);
void InitializeStatisticalIndeces(StatisticalIndeces& si);
void ComputeMeanVariance(StatisticalIndeces& si);
void ComputeCovarianceMatrix(StatisticalIndeces& si);

void FreeStatisticalIndeces(StatisticalIndeces& si);



#endif