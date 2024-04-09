#include <sigmod/exaustive.hh>
#include <sigmod/flags.hh>
#include <sigmod/seek.hh>
#include <sigmod/debug.hh>
#include <cassert>

void FilterIndexesByT(const Database& database, uint32_t& start_index, uint32_t& end_index, const float32_t l, const float32_t r) {
    // it's guaranteed that the database is ordered by C, T, fields
    // in such "order"
    start_index = SeekHigh(
        [&database](uint32_t i) { return database.at(i).T; },
        start_index, end_index, l
    );

    end_index = SeekLow(
        [&database](uint32_t i) { return database.at(i).T; },
        start_index, end_index, r
    ) + 1;
}

void FilterIndexesByC(const c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, const float32_t v) {
    start_index = C_map.at(v).first;
    end_index = C_map.at(v).second + 1;
}

void ExaustiveSearchByT(const Database& database, const Query& query, Scoreboard& scoreboard, const uint32_t start_index, const uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        if (!elegible_by_T(query, database.at(i)))
            continue;
        score_t score = distance(query, database.at(i));
        scoreboard.push(i, score);
    }
}

void ExaustiveSearch(const Database& database, const Query& query, Scoreboard& scoreboard, const uint32_t start_index, const uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        score_t score = distance(query, database.at(i));
        scoreboard.push(i, score);
    }
}

void SearchExaustive(const Database& database, Result& result, const Query& query) {
    // maximum distance in the front
    Scoreboard scoreboard;

    #ifdef DISATTEND_CHECKS
    const uint32_t query_type = NORMAL;
    #else
    const uint32_t query_type = (uint32_t) (query.query_type);
    #endif

    uint32_t start_index = 0;
    uint32_t end_index = database.length;

    switch(query_type) {
        case BY_T: {
            for (auto C_it : database.C_map) {
                start_index = C_it.second.first;
                end_index = C_it.second.second + 1;
                FilterIndexesByT(database, start_index, end_index, query.l, query.r);
                ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            }
            break;
        }; 
        case BY_C: {
            FilterIndexesByC(database.C_map, start_index, end_index, query.v);
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case BY_C_AND_T: {
            FilterIndexesByC(database.C_map, start_index, end_index, query.v);
            FilterIndexesByT(database, start_index, end_index, query.l, query.r);
            ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case NORMAL: {
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
    }

    assert (scoreboard.full());
    uint32_t rank = scoreboard.size() - 1;
    while(!scoreboard.empty()) {
        #ifdef FAST_INDEX
        result.data[rank] = scoreboard.top().index;
        #else
        result.data[rank] = database.indexes[scoreboard.top().index];
        #endif
        scoreboard.pop();
        rank -= 1;
    }
}
