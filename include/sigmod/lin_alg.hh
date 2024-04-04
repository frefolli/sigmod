#ifndef LIN_ALG_HH
#define LIN_ALG_HH

#include <config.hh>

const float32_t ComputeVectorNorm2(const float32_t vector[]);
void NormalizeVector(const float32_t* vector, float32_t* out, const uint32_t dimension);
float32_t* MallocVector(const uint32_t dimension);
float32_t* MallocVector(const uint32_t dimension, const float32_t initial_value);
inline void ScalarProduct(const float32_t* vector1, const float32_t* vector2, float32_t& out, const uint32_t dimension);
inline void ScalarDotVector(const float32_t scalar, const float32_t* vector, float32_t* out, const uint32_t dimension);
inline void SumVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension);
inline void DiffVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension);
inline void CopyVector(const float32_t* vector1, float32_t* out, const uint32_t dimension);
void FreeVector(float32_t* vector);

#endif