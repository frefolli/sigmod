#ifndef SCOREBOARD_HH
#define SCOREBOARD_HH

#include <sigmod/config.hh>
#include <sigmod/record.hh>
#include <sigmod/query.hh>
#include <queue>

inline score_t distance(const Query& query, const Record& record) {
    score_t sum = 0;
    for (uint32_t i = 0; i < vector_num_dimension; i++) {
        score_t m = query.fields[i] - record.fields[i];
        sum += (m * m);
    }
    return sum;
}

struct Candidate {
    uint32_t index;
    score_t score;

    bool operator<(const Candidate& other) const;
};

typedef std::priority_queue<Candidate, std::vector<Candidate>> Scoreboard;

inline void PushCandidate(Scoreboard& scoreboard, const Record& record, const Query& query, const uint32_t record_index) {
    scoreboard.emplace(Candidate({
        .index = record_index,
        .score = distance(query, record)
    }));
}

#endif
