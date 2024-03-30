#ifndef C_MAP_HH
#define C_MAP_HH

#include <sigmod/config.hh>
#include <map>
#include <utility>
#include <ostream>

typedef std::map<float32_t, std::pair<uint32_t, uint32_t>> c_map_t;

std::ostream& operator<<(std::ostream& out, c_map_t& C_map);

#endif
