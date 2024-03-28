#ifndef STATS_HH
#define STATS_HH

#include <string>
#include <ostream>
#include <sigmod/config.hh>

struct ScalarEntry {
    std::string what;
    float32_t mean;
    float32_t variance;
    float32_t min;
    float32_t max;

    template <typename Accessor>
    static ScalarEntry forArrayCellField(Accessor accessor, uint32_t length, std::string what) {
        float32_t acc = 0;
        float32_t acc2 = 0;
        float32_t min = accessor(0);
        float32_t max = accessor(0);

        for (uint32_t i = 1; i < length; i++) {
            float32_t value = accessor(i);
            acc += value;
            acc2 += (value * value);

            if (value < min)
                min = value;

            if (value > max)
                max = value;
        }

        float32_t mean = acc / length;
        float32_t var = (acc2 / length) - (mean * mean);

        return {
            .what = what,
            .mean = mean,
            .variance = var,
            .min = min,
            .max = max
        };
    }
};


std::ostream& operator<<(std::ostream& out, ScalarEntry& entry);

#endif
