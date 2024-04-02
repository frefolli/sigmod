#include <sigmod/random.hh>
#include <cstdlib>

uint32_t RandomUINT32T(uint32_t min, uint32_t max) {
    const uint32_t width = max - min;
    return (std::rand() % width) + min;
}
