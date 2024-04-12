#include <limits>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <iostream>
#include <cassert>

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

#define MAGIC_NUMBER 0x5FE6EB50C7B537A9
#define A 1.5f
#define B 0.5f
#define C 0.7221722172217222F
#define D 2.3894389438943895F

constexpr double Q_rsqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, MAGIC_NUMBER - (__builtin_bit_cast(uint64_t, number) >> 1));
  return y * (A - (number * B * y * y));
}

inline constexpr double Q_sqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, MAGIC_NUMBER - (__builtin_bit_cast(uint64_t, number) >> 1));
  return 1 / (y * (A - (number * B * y * y)));
}

constexpr double P_rsqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, MAGIC_NUMBER - (__builtin_bit_cast(uint64_t, number) >> 1));
  return y * C * (D - number * y * y);
}

inline constexpr double P_sqrt(double number) noexcept {
  double const y = __builtin_bit_cast(double, MAGIC_NUMBER - (__builtin_bit_cast(uint64_t, number) >> 1));
  return 1 / (y * C * (D - number * y * y));
}

inline double mse(const double* E, const double* F, uint32_t length) {
  double sum = 0.0;
  for (uint32_t i = 0; i < length; i++) {
    double d = E[i] - F[i];
    sum += d*d;
  }
  return sum / length;
}

int main() {
    std::srand(std::time(0));
    const uint32_t SIZE = 10000000;
    const double MIN = 0;
    const double MAX = 100;
    double* Xs = (double*) malloc(sizeof(double) * SIZE);
    assert(Xs != nullptr);
    RandomDoubleBatch(MIN, MAX, Xs, SIZE);
    
    double* EYs = (double*) malloc(sizeof(double) * SIZE);
    assert(EYs != nullptr);
    double* QYs = (double*) malloc(sizeof(double) * SIZE);
    assert(QYs != nullptr);
    double* PYs = (double*) malloc(sizeof(double) * SIZE);
    assert(PYs != nullptr);

    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < SIZE; i++) {
        EYs[i] = std::sqrt(Xs[i]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    ETA("sqrt_x", end, start);
    
    start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < SIZE; i++) {
        QYs[i] = Q_sqrt(Xs[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    ETA("qsqrt_x", end, start);
    
    start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < SIZE; i++) {
        PYs[i] = P_sqrt(Xs[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    ETA("psqrt_x", end, start);

    std::cout << "MSE_SQRT_Q := " << mse(EYs, QYs, SIZE) << std::endl;
    std::cout << "MSE_SQRT_P := " << mse(EYs, PYs, SIZE) << std::endl;

    free(Xs);
    free(EYs);
    free(QYs);
    free(PYs);
}
