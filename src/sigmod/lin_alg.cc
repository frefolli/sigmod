#include <sigmod/lin_alg.hh>
#include <cmath>

const float32_t ComputeVectorNorm2(const float32_t* vector, const uint32_t dimension){
    float32_t sum = 0;
    for (int32_t i = 0; i < dimension; i++) {
        sum = vector[i] * vector[i];
    }
    return sqrt(sum);
}  

void NormalizeVector(const float32_t* vector, float32_t* out, const uint32_t dimension) {
    float32_t norm2 = ComputeVectorNorm2(vector);
    for (int32_t i = 0; i < dimension; i++) {
        out[i] = vector[i] / norm2;
    }
}

float32_t* MallocVector(const uint32_t dimension){
    float32_t* vector = (float32_t*) std::malloc(sizeof(float32_t) * dimension);
    return vector;
}    

float32_t* MallocVector(const uint32_t dimension, const float32_t initial_value){
    float32_t* vector = (float32_t*) std::malloc(sizeof(float32_t) * dimension);
    for (int32_t i = 0; i < dimension; i++) {
        vector[i] = initial_value;
    }
    return vector;
}    

inline void ScalarProduct(const float32_t* vector1, const float32_t* vector2, float32_t &out, const uint32_t dimension) {
    out = 0; 

    for (int32_t i = 0; i < dimension; i++) {
        out += vector1[i] * vector2[i];
    }
}

inline void ScalarDotVector(const float32_t scalar, const float32_t* vector, float32_t* out, const uint32_t dimension) {
    
    for (int32_t i = 0; i < dimension; i++) {
        out[i] = vector[i] * scalar;
    }
}

inline void SumVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension) {
    for (int32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i] + vector2[i];
    }
}

inline void DiffVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension) {
    for (int32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i] - vector2[i];
    }
}

inline void CopyVector(const float32_t* vector1, float32_t* out, const uint32_t dimension) {
    for (int32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i];
    }
}

void FreeVector(float32_t* vector) {
    if(vector != nullptr) {
        std::free(vector);
    }
}