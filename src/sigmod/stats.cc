#include <sigmod/stats.hh>

std::ostream& operator<<(std::ostream& out, ScalarEntry& entry) {
    out << entry.what
        << " := (mean " << entry.mean
        << ") . (var " << entry.variance
        << ") . (min " << entry.min
        << ") . (max " << entry.max
        << ")";
    return out;
}
