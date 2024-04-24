#ifndef C_MAP_HH
#define C_MAP_HH

#include <sigmod/config.hh>
#include <unordered_map>
#include <utility>
#include <ostream>

/* C_map is a mapping of C -> [start, end] positions
 * **note the inclusion of the end**
 * */
typedef std::unordered_map<float32_t, std::pair<uint32_t, uint32_t>> c_map_t;

std::ostream& operator<<(std::ostream& out, const c_map_t& C_map);

#endif
