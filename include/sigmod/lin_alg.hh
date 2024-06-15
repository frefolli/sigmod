#ifndef LIN_ALG_HH
#define LIN_ALG_HH
/** @file lin_alg.hh */

#include <sigmod/config.hh>


float32_t* MallocVector(const uint32_t dimension);
float32_t* MallocVector(const uint32_t dimension, const float32_t initial_value);

float32_t** MallocMatrix(const uint32_t rows, const uint32_t columns);
float32_t** MallocMatrix(const uint32_t rows, const uint32_t columns, const float32_t initial_value);

const float32_t ComputeVectorNorm2(const float32_t* vector, const uint32_t dimension);
void NormalizeVector(const float32_t* vector, float32_t* out, const uint32_t dimension);

void ScalarProduct(const float32_t* vector1, const float32_t* vector2, float32_t& out, const uint32_t dimension);
void ScalarDotVector(const float32_t scalar, const float32_t* vector, float32_t* out, const uint32_t dimension);
void SumVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension);
void DiffVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension);

void MatrixProduct(const float32_t** matrix1, const uint32_t rows, const uint32_t columns1, const float32_t** matrix2, const uint32_t columns2, float32_t** out);

void CopyVectorFrom(const float32_t* vector1, float32_t* out, const uint32_t dimension);
void CopyMatrixFrom(const float32_t** matrix, float32_t** out, const uint32_t rows, const uint32_t columns);

void FreeVector(float32_t* vector);
void FreeMatrix(float32_t** matrix);

#endif
