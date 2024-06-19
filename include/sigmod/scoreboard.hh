#ifndef SCOREBOARD_HH
#define SCOREBOARD_HH
/** @file scoreboard.hh */

#include <sigmod/config.hh>
#include <sigmod/flags.hh>
#include <sigmod/debug.hh>
#include <sigmod/record.hh>
#include <sigmod/query.hh>
#include <vector>

#ifndef FAST_DISTANCE
#include <cmath>
#endif

#ifdef FAST_SQRT
#define QUACKE3_MAGIC_NUMBER 0x5FE6EB50C7B537A9
#define QUACKE3_A 1.5f
#define QUACKE3_B 0.5f

template<typename F, typename T>
inline constexpr T bit_cast(F obj) noexcept {
  T* ptr = (T*) &obj;
  return *ptr;
}

inline constexpr double quacke3_sqrt(double number) noexcept {
  double const y = bit_cast<uint64_t, double>(QUACKE3_MAGIC_NUMBER - (bit_cast<double, uint64_t>(number) >> 1));
  return 1 / (y * (QUACKE3_A - (number * QUACKE3_B * y * y)));
}

#define QUACKE2_MAGIC_NUMBER 0x5FE6EB50C7B537A9
#define QUACKE2_C 0.7221722172217222F
#define QUACKE2_D 2.3894389438943895F

inline constexpr double quacke2_sqrt(double number) noexcept {
  double const y = bit_cast<uint64_t, double>(QUACKE3_MAGIC_NUMBER - (bit_cast<double, uint64_t>(number) >> 1));
  return 1 / (y * QUACKE2_C * (QUACKE2_D - number * y * y));
}
#endif

/* 
* In case we reduce dataset using RP distance changed by sqrt(d/k)*original distance,
* were d is initial dimension and k is final dimension.
*/
template <typename WFA, typename WFB>
inline score_t distance(const WFA& query, const WFB& record) {
    #ifdef TRACK_DISTANCE_COMPUTATIONS
        SIGMOD_DISTANCE_COMPUTATIONS++;
    #endif
    score_t sum = 0;
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        score_t m = query.fields[i] - record.fields[i];
        sum += (m * m);
    }
    #ifdef FAST_DISTANCE
        return sum;
    #else
        #ifndef FAST_SQRT
            return std::sqrt(sum);
        #else
            return quacke3_sqrt(sum);
        #endif
    #endif
}

template <typename WithFields>
score_t first_metric(WithFields& with_fields) {
    score_t sum = 0.0;
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        sum += with_fields.fields[i] * with_fields.fields[i];
    }
    #ifdef FAST_DISTANCE
        return sum;
    #else
        #ifndef FAST_SQRT
            return std::sqrt(sum);
        #else
            return quacke3_sqrt(sum);
        #endif
    #endif
}

template <typename WithFields>
score_t second_metric(WithFields& with_fields) {
    score_t gamma = 0.0;
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        gamma += with_fields.fields[i];
    }
    gamma /= actual_vector_size;

    score_t sum = 0.0;
    score_t val = 0.0;

    for (uint32_t i = 0; i < actual_vector_size; i++) {
        val = with_fields.fields[i] - gamma;
        sum += val * val;
    }
    #ifdef FAST_DISTANCE
        return sum;
    #else
        #ifndef FAST_SQRT
            return std::sqrt(sum);
        #else
            return quacke3_sqrt(sum);
        #endif
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
        inline void pushs(const uint32_t index, const score_t score) {
          if (!has(index)) {
              if (full()) {
                  if (score < top().score) {
                      pop();
                      add(index, score);
                  }
              } else {
                  add(index, score);
              }
          }
        }
        void clear();
};

#endif
