#include <sigmod/random.hh>
#include <cstdlib>

uint32_t RandomUINT32T(const uint32_t min, const uint32_t max) {
    const uint32_t width = max - min;
    return (std::rand() % width) + min;
}

float32_t RandomFLOAT32T(const float32_t min, const float32_t max) {
    const float32_t width = RAND_MAX / (max - min);
    return (static_cast<float32_t>(std::rand()) / width) + min;
}
