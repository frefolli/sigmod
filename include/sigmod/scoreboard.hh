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

    Candidate(uint32_t index, score_t score);
};

// typedef std::priority_queue<Candidate, std::vector<Candidate>> Scoreboard;

class Scoreboard {
    private:
        std::vector<Candidate> board = {};
    public:
        uint32_t size();
        Candidate& top();
        void pop();
        void add(uint32_t index, score_t score);
        void consider(Candidate& candidate);
        bool has(uint32_t index);
        bool empty();
};

inline void PushCandidate(Scoreboard& scoreboard, const Record& record, const Query& query, const uint32_t record_index) {
    const score_t score = distance(query, record);
    if (scoreboard.size() == k_nearest_neighbors) {
        if (score < scoreboard.top().score) {
            scoreboard.add(record_index, score);
            if (scoreboard.size() != k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    } else {
        scoreboard.add(record_index, score);
    }
}

#endif
