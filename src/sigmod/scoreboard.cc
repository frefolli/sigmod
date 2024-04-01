#include <sigmod/scoreboard.hh>

bool Candidate::operator<(const Candidate& other) const {
    return score < other.score;
}

uint32_t Scoreboard::size() {
    return board.size();
}

Candidate& Scoreboard::top() {
    return board.back();
}

void Scoreboard::pop() {
    board.pop_back();
}

void Scoreboard::add(uint32_t index, score_t score) {
    if (has(index))
        return;

    auto it = board.begin();
    while(it != board.end() && it->score < score)
        it++;

    board.emplace(it, index, score);
}

bool Scoreboard::has(uint32_t index) {
    for (auto it = board.begin(); it != board.end(); it++) {
        if (it->index == index)
            return true;
    }
    return false;
}

bool Scoreboard::empty() {
    return board.size() == 0;
}

Candidate::Candidate(uint32_t index, score_t score) :
    index(index), score(score) {}