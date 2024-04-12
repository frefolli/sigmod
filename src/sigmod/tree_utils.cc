#include <sigmod/tree_utils.hh>
#include <sigmod/scoreboard.hh>
#include <queue>

uint32_t FindFurthestPoint(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end, const uint32_t target) {
    uint32_t furthest = start;
    score_t furthest_score = distance(database.at(indexes[target]), database.at(indexes[furthest]));
    for (uint32_t i = start + 1; i < end; i++) {
        score_t score = distance(database.at(indexes[target]), database.at(indexes[i]));
        if (score > furthest_score) {
            furthest_score = score;
            furthest = i;
        }
    }
    return furthest;
}

auto compareFunc = [](const item_t& a, const item_t& b) {
  return a.second < b.second;
};

void ReorderByCoupledValue(uint32_t* indexes, score_t* coupled_values, uint32_t length) {
  std::priority_queue<item_t, std::vector<item_t>, decltype(compareFunc)> items (compareFunc);
  for (uint32_t i = 0; i < length; i++) {
    items.push({indexes[i], coupled_values[i]});
  }
  for (uint32_t i = length; i > 0; i--) {
    indexes[i - 1] = items.top().first;
    coupled_values[i - 1] = items.top().second;
    items.pop();
  }
}


uint32_t MaximizeSpread(const Database& database, uint32_t* indexes, const uint32_t start, const uint32_t end) {
  uint32_t best_dim = 0;
  float32_t cur_min = database.at(start).fields[best_dim];
  float32_t cur_max = database.at(start).fields[best_dim];
  for (uint32_t i = start + 1; i <= end; i++) {
    const float32_t val = database.at(indexes[i]).fields[best_dim];
    if (val < cur_min)
        cur_min = val;
    if (val > cur_max)
        cur_max = val;
  }

  float32_t best_min = cur_min;
  float32_t best_max = cur_max;

  for (uint32_t dim = 1; dim < actual_vector_size; dim++) {
    cur_min = database.at(start).fields[dim];
    cur_max = database.at(start).fields[dim];
    for (uint32_t i = start + 1; i <= end; i++) {
      const float32_t val = database.at(indexes[i]).fields[best_dim];
      if (val < cur_min)
          cur_min = val;
      if (val > cur_max)
          cur_max = val;
    }
    if (cur_max - cur_min > best_max - best_min) {
      best_min = cur_min;
      best_max = cur_max;
      best_dim = dim;
    }
  }

  return best_dim;
}