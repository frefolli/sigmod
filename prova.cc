// #include <bit>
#include <limits>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <iostream>

#define SHOW(x) std::cout << #x " := " << x << std::endl;
#define ETA(msg, end, start) std::cout << "# | " << msg << " | Elapsed (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

double RandomDouble(const double min, const double max) {
    const double width = RAND_MAX / (max - min);
    return (static_cast<double>(std::rand()) / width) + min;
}

void RandomDoubleBatch(const double min, const double max, double* buffer, uint32_t length) {
    const double width = RAND_MAX / (max - min);
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = (static_cast<double>(std::rand()) / width) + min;
    }
}

constexpr double Q_rsqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, 0x5FE6EC85E7DE30DA - (__builtin_bit_cast<uint64_t>(number) >> 1));
  return y * (1.5f - (number * 0.5f * y * y));
}

inline constexpr double Q_sqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, 0x5FE6EC85E7DE30DA - (__builtin_bit_cast<uint64_t>(number) >> 1));
  return 1 / (y * (1.5f - (number * 0.5f * y * y)));
}

int main() {
    std::srand(std::time(0));
    const uint32_t SIZE = 1000000;
    const double MIN = 0;
    const double MAX = 100;
    double* Xs = (double*) malloc(sizeof(double) * SIZE);
    RandomDoubleBatch(MIN, MAX, Xs, SIZE);

    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < SIZE; i++) {
        double sqrt_x = std::sqrt(Xs[i]);
        double rsqrt_x = 1 / sqrt_x;
    }
    auto end = std::chrono::high_resolution_clock::now();
    ETA("sqrt_x", end, start);
    
    start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < SIZE; i++) {
        double q_rsqrt_x = Q_rsqrt(Xs[i]);
        double q_sqrt_x = 1 / q_rsqrt_x;
    }
    end = std::chrono::high_resolution_clock::now();
    ETA("qsqrt_x", end, start);

    std::cout << std::sqrt(23456.4567) << std::endl;
    std::cout << Q_sqrt(23456.4567) << std::endl;

    free(Xs);
}
