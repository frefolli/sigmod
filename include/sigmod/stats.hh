#ifndef STATS_HH
#define STATS_HH
/** @file stats.hh */

#include <string>
#include <unordered_map>
#include <ostream>
#include <sigmod/config.hh>

struct ScalarEntry {
    std::string what;
    float32_t mean;
    float32_t variance;
    float32_t min;
    float32_t max;

    template <typename Accessor>
    static ScalarEntry forArrayCellField(const Accessor accessor, const uint32_t length, const std::string what) {
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

struct CategoricalEntry {
    std::string what;
    std::unordered_map<uint32_t, uint32_t> counts;

    template <typename Accessor>
    static CategoricalEntry forArrayCellField(const Accessor accessor, const uint32_t length, const std::string what) {
        CategoricalEntry res = {
            .what = what,
            .counts = {}
        };

        for (uint32_t i = 0; i < length; i++) {
            res.counts[accessor(i)] += 1;
        }

        return res;
    }
};

std::ostream& operator<<(std::ostream& out, const ScalarEntry entry);
std::ostream& operator<<(std::ostream& out, const CategoricalEntry entry);

#endif
