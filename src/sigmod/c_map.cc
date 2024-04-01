#include <sigmod/c_map.hh>

std::ostream& operator<<(std::ostream& out, c_map_t& C_map) {
    out << " C_map :=";
    for (auto it : C_map) {
        out << " (" << it.first
            << " . (" << it.second.first
            << " . " << it.second.second
            << "))";
    }
    return out;
}
