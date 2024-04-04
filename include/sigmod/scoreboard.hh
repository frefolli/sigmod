#ifndef SCOREBOARD_HH
#define SCOREBOARD_HH

#include <sigmod/config.hh>
#include <sigmod/record.hh>
#include <sigmod/query.hh>
#include <vector>

template <typename WFA, typename WFB>
inline score_t distance(const WFA& query, const WFB& record) {
    score_t sum = 0;
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        score_t m = query.fields[i] - record.fields[i];
        sum += (m * m);
    }
    return sum;
}

struct Candidate {
    uint32_t index;
    score_t score;

    Candidate(const uint32_t index, const score_t score);
};

/*
* I think we can convert this vector to a Candidate& carray because
* all insertions follow insertion sort and we don't have ever more than k_nearest_neighbors
* across the sigmod program
*/
struct Scoreboard {
    private:
        std::vector<Candidate> board = {};
    public:
        uint32_t size() const;
        const Candidate& top() const;
        void pop();
        void add(const uint32_t index, const score_t score);
        void consider(const Candidate& candidate);
        void update(const Scoreboard& input);
        bool has(const uint32_t index) const;
        bool empty() const;
        bool full() const;
        inline void push(const uint32_t index, const score_t score) {
          if (full()) {
              if (score < top().score) {
                  pop();
                  add(index, score);
              }
          } else {
              add(index, score);
          }
        }
};

#endif
