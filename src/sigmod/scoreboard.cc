#include <sigmod/scoreboard.hh>

bool Candidate::operator<(const Candidate& other) const {
    return score < other.score;
}
