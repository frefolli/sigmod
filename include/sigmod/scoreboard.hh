#ifndef SCOREBOARD_HH
#define SCOREBOARD_HH

#include <sigmod/config.hh>
#include <sigmod/flags.hh>
#include <sigmod/debug.hh>
#include <sigmod/record.hh>
#include <sigmod/query.hh>
#include <vector>

#ifndef FAST_DISTANCE
#include <cmath>
#endif

/* 
* In case we reduce dataset using RP distance changed by sqrt(d/k)*original distance,
* were d is initial dimension and k is final dimension.
*/
template <typename WFA, typename WFB>
inline score_t distance(const WFA& query, const WFB& record) {
    SIGMOD_DISTANCE_COMPUTATIONS++;
    score_t sum = 0;
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        score_t m = query.fields[i] - record.fields[i];
        sum += (m * m);
    }
    #ifdef FAST_DISTANCE
    return sum;
    #else
    return std::sqrt(sum);
    #endif
}

inline bool check_if_elegible_by_T(const Query& query, const Record& record) {
    if (query.query_type == BY_C || query.query_type == NORMAL)
      return true;
    return (query.l <= record.T && query.r >= record.T);
}

inline bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l <= record.T && query.r >= record.T);
}

struct Candidate {
    uint32_t index;
    score_t score;

    Candidate(const uint32_t index = -1, const score_t score = -1);
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
        const Candidate& furthest() const;
        const Candidate& nearest() const;
        inline const Candidate& top() const {
            return furthest();
        };
        inline const Candidate& bottom() const {
            return nearest();
        };
        void pop();
        void add(const uint32_t index, const score_t score);
        void consider(const Candidate& candidate);
        void update(const Scoreboard& input);
        bool has(const uint32_t index) const;
        bool empty() const;
        bool full() const;
        inline bool not_full() const { return !full(); }
        inline void pushf(const uint32_t index, const score_t score) {
          // assumes score < furthest().score has been done
          if (full()) {
              pop();
          }
          add(index, score);
        }
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
        void clear();
};

#endif
