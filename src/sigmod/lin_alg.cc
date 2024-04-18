#include <sigmod/lin_alg.hh>
#include <sigmod/memory.hh>
#include <cmath>

float32_t* MallocVector(const uint32_t dimension){
    float32_t* vector = smalloc<float32_t>(dimension);
    return vector;
}    

float32_t* MallocVector(const uint32_t dimension, const float32_t initial_value){
    float32_t* vector = MallocVector(dimension);
    for (int32_t i = 0; i < dimension; i++) {
        vector[i] = initial_value;
    }
    return vector;
}    

float32_t** MallocMatrix(const uint32_t rows, const uint32_t columns) {
    float32_t** matrix = smalloc<float32_t*>(rows);
    for(uint32_t i = 0; i < rows; i++)
        matrix[i] = smalloc<float32_t>(columns);
    return matrix;
}    

float32_t** MallocMatrix(const uint32_t rows, const uint32_t columns, const float32_t initial_value) {
    float32_t** matrix = MallocMatrix(rows, columns);
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < rows; j++) {
            matrix[i][j] = initial_value;
        }
    }
    return matrix;
}    

const float32_t ComputeVectorNorm2(const float32_t* vector, const uint32_t dimension){
    float32_t sum = 0;
    for (uint32_t i = 0; i < dimension; i++) {
        sum = vector[i] * vector[i];
    }
    return sqrt(sum);
}  

void NormalizeVector(const float32_t* vector, float32_t* out, const uint32_t dimension) {
    float32_t norm2 = ComputeVectorNorm2(vector, dimension);
    for (uint32_t i = 0; i < dimension; i++) {
        out[i] = vector[i] / norm2;
    }
}


void ScalarProduct(const float32_t* vector1, const float32_t* vector2, float32_t &out, const uint32_t dimension) {
    out = 0; 

    for (uint32_t i = 0; i < dimension; i++) {
        out += vector1[i] * vector2[i];
    }
}

void ScalarDotVector(const float32_t scalar, const float32_t* vector, float32_t* out, const uint32_t dimension) {
    
    for (uint32_t i = 0; i < dimension; i++) {
        out[i] = vector[i] * scalar;
    }
}

void SumVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension) {
    for (uint32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i] + vector2[i];
    }
}

void DiffVectors(const float32_t* vector1, const float32_t* vector2, float32_t* out, const uint32_t dimension) {
    for (uint32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i] - vector2[i];
    }
}

void MatrixProduct(const float32_t** matrix1, const uint32_t rows, const uint32_t columns1, const float32_t** matrix2,  const uint32_t columns2, float32_t** out) {
    float32_t sum = 0;
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < columns2; j++) {
            sum = 0;
            for (uint32_t k = 0; k < columns1; k++) {
                sum += matrix1[i][k] * matrix2[k][j];
            }
            out[i][j] = sum;
        }
    }
}

void CopyVectorFrom(const float32_t* vector1, float32_t* out, const uint32_t dimension) {
    for (uint32_t i = 0; i < dimension; i++) {
        out[i] = vector1[i];
    }
}

void CopyMatrixFrom(const float32_t** matrix, float32_t** out, const uint32_t rows, const uint32_t columns) {
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < columns; j++) {
            out[i][j] = matrix[i][j];
        }
    }
}

void FreeVector(float32_t* vector) {
    if(vector != nullptr) {
        free(vector);
    }
}

void FreeMatrix(float32_t** matrix) {
    if(matrix != nullptr) {
        free(matrix);
    }
}
