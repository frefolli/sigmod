#include <sigmod/random_projection.hh>
#include <cstdlib>
#include <random>
#include <sigmod/lin_alg.hh>

const float32_t** GenerateProjectionMatrix(
        const uint32_t final_dimension, 
        const uint32_t initial_dimension) {

    std::random_device rd;     // Only used once to initialise (seed) engine
    std::mt19937 rng(rd());    // Random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<uint8_t> uni(0, 5); // Guaranteed unbiased

    float32_t** R = MallocMatrix(initial_dimension, final_dimension);

    uint8_t random = 0; 
    for (uint32_t i = 0; i < initial_dimension; i++) {
        for (uint32_t j = 0; j < final_dimension; i++) {
            switch (uni(rng)) {
            case 1:
                random = 1;
                break;
            case 6:
                random = -1;
                break;
            default:
                random = 0;
                break;
            }
            R[i][j] = sqrt(3) * random;
        }
    }
    return R;
}

const float32_t** RamdomProjection(
        float32_t** dataset_matrix, 
        const uint32_t n_observation, 
        const uint32_t dimension, 
        const uint32_t final_dimension) {

    const float32_t** prj_matrix = GenerateProjectionMatrix(final_dimension, dimension);
    //float32_t** dataset_prj = MallocMatrix(n_observation, final_dimension);
    MatrixProduct(dataset_matrix, n_observation, dimension, prj_matrix, final_dimension, dataset_matrix);
    //CopyMatrixFrom(dataset_prj, dataset_matrix, n_observation, final_dimension);
    //FreeProjectionMatrix(dataset_prj);
    return prj_matrix;
}

void RamdomProjectionGivenProjMatrix(
        float32_t** dataset_matrix, 
        const uint32_t n_observation, 
        const uint32_t dimension, 
        const float32_t** prj_matrix, 
        const uint32_t final_dimension) {

    //float32_t** dataset_prj = MallocMatrix(n_observation, final_dimension);
    MatrixProduct(dataset_matrix, n_observation, dimension, prj_matrix, final_dimension, dataset_matrix);
    // CopyMatrixFrom(dataset_prj, dataset_matrix, n_observation, final_dimension);
    //FreeProjectionMatrix(dataset_prj);
}


void FreeProjectionMatrix(float32_t** matrix) {
    FreeMatrix(matrix);
}