#include <sigmod/stats.hh>

std::ostream& operator<<(std::ostream& out, ScalarEntry entry) {
    out << entry.what
        << " := (mean " << entry.mean
        << ") . (var " << entry.variance
        << ") . (min " << entry.min
        << ") . (max " << entry.max
        << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, CategoricalEntry entry) {
    out << entry.what << " :=";
    for (auto it : entry.counts) {
        out << " (" << it.first << " . " << it.second << ")";
    }
    return out;
}
