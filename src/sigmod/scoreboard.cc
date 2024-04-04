#include <sigmod/scoreboard.hh>
#include <sigmod/flags.hh>

uint32_t Scoreboard::size() const {
    return board.size();
}

const Candidate& Scoreboard::top() const {
    return board.back();
}

void Scoreboard::pop() {
    board.pop_back();
}

void Scoreboard::add(uint32_t index, score_t score) {
    #ifdef SCOREBOARD_ALWAYS_CHECK_DUPLICATES
    if (has(index))
      return;
    #endif

    auto it = board.begin();
    while(it != board.end() && it->score < score)
        it++;

    board.emplace(it, index, score);
}

bool Scoreboard::has(uint32_t index) const {
    for (auto it = board.begin(); it != board.end(); it++) {
        if (it->index == index)
            return true;
    }
    return false;
}

bool Scoreboard::empty() const {
    return board.size() == 0;
}

Candidate::Candidate(uint32_t index, score_t score) :
    index(index), score(score) {}

void Scoreboard::consider(Candidate& candidate) {
    if (full()) {
        if (candidate.score < board.back().score) {
            pop();
            add(candidate.index, candidate.score);
        }
    } else {
        add(candidate.index, candidate.score);
    }
}

void Scoreboard::update(const Scoreboard& input) {
    for (auto candidate : input.board) {
        consider(candidate);
    }
}

bool Scoreboard::full() const {
    return board.size() == k_nearest_neighbors;
}
