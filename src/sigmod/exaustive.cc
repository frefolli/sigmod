#include <sigmod/exaustive.hh>
#include <sigmod/seek.hh>

bool elegible_by_T(const Query& query, const Record& record) {
    return (query.l >= record.T && record.T <= query.r);
}

void FilterIndexesByT(Database& database, uint32_t& start_index, uint32_t& end_index, float32_t l, float32_t r) {
    // it's guaranteed that the database is ordered by C, T, fields
    // in such "order"
    start_index = SeekHigh(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, l
    );

    end_index = SeekLow(
        [&database](uint32_t i) { return database.records[i].T; },
        start_index, end_index, r
    );
}

void FilterIndexesByC(c_map_t& C_map, uint32_t& start_index, uint32_t& end_index, float32_t v) {
    start_index = C_map[v].first;
    end_index = C_map[v].second + 1;
}

void ExaustiveSearchByT(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        if (elegible_by_T(query, database.records[i])) {
            PushCandidate(scoreboard, database.records[i], query, i);
            if (scoreboard.size() > k_nearest_neighbors) {
                scoreboard.pop();
            }
        }
    }
}

void ExaustiveSearch(Database& database, Query& query, Scoreboard& scoreboard, uint32_t start_index, uint32_t end_index) {
    for (uint32_t i = start_index; i < end_index; i++) {
        PushCandidate(scoreboard, database.records[i], query, i);
        if (scoreboard.size() > k_nearest_neighbors) {
            scoreboard.pop();
        }
    }
}

void SearchExaustive(Result& result, Database& database, c_map_t& C_map, Query& query) {
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
            for (auto C_it : C_map) {
                start_index = C_it.second.first;
                end_index = C_it.second.second;
                FilterIndexesByT(database, start_index, end_index, query.l, query.r);
                ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            }
            break;
        }; 
        case BY_C: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case BY_C_AND_T: {
            FilterIndexesByC(C_map, start_index, end_index, query.v);
            FilterIndexesByT(database, start_index, end_index, query.l, query.r);
            ExaustiveSearchByT(database, query, scoreboard, start_index, end_index);
            break;
        }; 
        case NORMAL: {
            ExaustiveSearch(database, query, scoreboard, start_index, end_index);
            break;
        }; 
    }

    uint32_t i = k_nearest_neighbors - 1;
    while(!scoreboard.empty()) {
        result.data[i] = scoreboard.top().index;
        scoreboard.pop();
        i -= 1;
    }
}
