#include <sigmod/database.hh>
#include <sigmod/stats.hh>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <sigmod/debug.hh>
#include <cstring>
#include <sigmod/flags.hh>
#include <sigmod/tweaks.hh>
#include <sigmod/dimensional_reduction.hh>
#include <sigmod/lin_alg.hh>
#include <sigmod/memory.hh>
#include <cmath>
#include <sigmod/scoreboard.hh>
#include <omp.h>

Database ReadDatabase(const std::string input_path) {
    FILE* dbfile = fopen(input_path.c_str(), "rb");
    
    uint32_t db_length;
    fread(&db_length, sizeof(uint32_t), 1, dbfile);

    Record* records = (Record*) std::malloc(sizeof(Record) * db_length);
    RawRecord* records_entry_point = (RawRecord*) records;
    uint32_t records_to_read = db_length;
    while(records_to_read > 0) {
        uint32_t this_batch = BATCH_SIZE;
        if (this_batch > records_to_read) {
            this_batch = records_to_read;
        }
        fread(records_entry_point, sizeof(RawRecord), this_batch, dbfile);
        records_to_read -= this_batch;
        records_entry_point += this_batch;
    }
    fclose(dbfile);

    records_entry_point -= 1;
    for (uint32_t i = db_length - 1; i > 0; i--) {
        std::memmove(records + i, records_entry_point, sizeof(RawRecord));
        records_entry_point -= 1;
        records[i].index = i;
    }
    records[0].index = 0;

    return {
        .length = db_length,
        .records = records,
        .C_map = {},
    };
}

void FreeDatabase(Database& database) {
    if (database.records == nullptr)
        return;
    free(database.records);
    database.records = nullptr;
    database.length = 0;
}

bool operator<(const Record& a, const Record& b) {
    if (a.C != b.C)
        return (a.C < b.C);
    if (a.T != b.T)
        return (a.T < b.T);
    for (uint32_t i = 0; i < actual_vector_size; i++) {
        if (a.fields[i] != b.fields[i])
            return (a.fields[i] < b.fields[i]);
    }
    return true;
}

void IndexDatabase(Database& database) {
    std::sort(database.records, database.records + database.length,
              [&database](const Record& a, const Record& b) {
        return a < b;
    });
    
    float32_t cur_C = database.at(0).C;
    uint32_t cur_start = 0;
    uint32_t cur_end = 0;
    for (uint32_t i = 1; i < database.length; i++) {
        float32_t Ci = database.at(i).C;
        if (Ci == cur_C) {
            cur_end += 1;
        } else {
            database.C_map[cur_C] = {cur_start, cur_end};
            cur_C = Ci;
            cur_start = i;
            cur_end = cur_start;
        }
    }
    database.C_map[cur_C] = {cur_start, cur_end};
}

void ClusterizeDatabase(const Database& database) {
    const uint32_t ITERATIONS = 1;
    for (auto it : database.C_map) {
        const uint32_t start = it.second.first;
        const uint32_t end = it.second.second + 1;
        const uint32_t length = end - start;
        if (length > 25) {
            const uint32_t n_of_clusters = std::sqrt(length);
            LogTime("Clusterizing C#"
		      + std::to_string((uint32_t) it.first)
		      + " of length := "
		      + std::to_string(length)
		      + " and n_of_clusters := "
		      + std::to_string(n_of_clusters));
            uint32_t* beholds = smalloc<uint32_t>(length);
            Record* centroids = smalloc<Record>(n_of_clusters);

	    LogTime("Initializing Centroids");
            #pragma omp parallel
            {
                #pragma omp for
                for (uint32_t i = 0; i < n_of_clusters; i++) {
                    for (uint32_t j = 0; j < actual_vector_size; j++) {
                        centroids[i].fields[j] = 0;
                    }
                    // C is used to count points
                    // which are anchored to a centroid
                    centroids[i].C = 0;
                }
                #pragma omp barrier
	    }

	    LogTime("Dummy iteration");
                // DUMMY ITERATION
                // initialization of centroids: even spread over modulo
                for (uint32_t i = 0; i < length; i++) {
                    uint32_t centroid = i % n_of_clusters;
                    beholds[i] = centroid;
                    Record& record = database.records[start + i];
                    {
                        for (uint32_t j = 0; j < actual_vector_size; j++) {
                            centroids[centroid].fields[j] += record.fields[j];
                        }
                        centroids[centroid].C += 1;
                    }
                }

	    LogTime("Meaning centroids");
            #pragma omp parallel
            {
                // compute mean of cumulated coordinates
                #pragma omp for
                for (uint32_t i = 0; i < n_of_clusters; i++) {
                    for (uint32_t j = 0; j < actual_vector_size; j++) {
                        centroids[i].fields[j] /= centroids[i].C;
                    }
                }
		#pragma omp barrier
	    }
	    LogTime("Meaned centroids");

	    for (uint32_t iteration = 0; iteration < ITERATIONS; iteration++) {
            #pragma omp parallel
            {
                    // FULL ITERATION
                    // computing the nearest centroid
                    #pragma omp for
                    for (uint32_t i = 0; i < length; i++) {
                        Record& record = database.records[start + i];
                        score_t min_dist = distance(centroids[0], record);
                        beholds[i] = 0;
                        for (uint32_t j = 1; j < n_of_clusters; j++) {
                            score_t dist = distance(centroids[j], record);
                            if (dist < min_dist) {
                                min_dist = dist;
                                beholds[i] = j;
                            }
                        }
                    }

                    #pragma omp barrier
                    // reset centroid
                    #pragma omp for
                    for (uint32_t i = 0; i < n_of_clusters; i++) {
                        for (uint32_t j = 0; j < actual_vector_size; j++) {
                            centroids[i].fields[j] = 0;
                        }
                        centroids[i].C = 0;
                    }
                    #pragma omp barrier
	    }

	            LogTime("Filling centroids");
                    // refill centroid data
                    for (uint32_t i = 0; i < length; i++) {
                        uint32_t centroid = beholds[i];
                        Record& record = database.records[start + i];
                        {
                            for (uint32_t j = 0; j < actual_vector_size; j++) {
                                centroids[centroid].fields[j] += record.fields[j];
                            }
                            centroids[beholds[i]].C += 1;
                        }
                    }
	            LogTime("Filled centroids");
            
	    #pragma omp parallel
            {
                    // compute mean of cumulated coordinates
                    #pragma omp for
                    for (uint32_t i = 0; i < n_of_clusters; i++) {
                        for (uint32_t j = 0; j < actual_vector_size; j++) {
                            centroids[i].fields[j] /= centroids[i].C;
                        }
                    }
	    }
            }

	    /*
            // print counts
            for (uint32_t i = 0; i < n_of_clusters; i++) {
                std::cout << "len(centroids["
                          << i << "]) = "
                          << centroids[i].C << std::endl;
            }
	    */

            free(centroids);
            free(beholds);
        }
    }
}

void StatsDatabase(const Database& database) {
    std::cout << CategoricalEntry::forArrayCellField(
        [&database](uint32_t i) { return database.at(i).C; },
        database.length, "record.C"
    ) << std::endl;

    std::cout << ScalarEntry::forArrayCellField(
        [&database](uint32_t i) { return database.at(i).T; },
        database.length, "record.T"
    ) << std::endl;

    /*
    for (uint32_t j = 0; j < actual_vector_size; j++) {
        std::cout << ScalarEntry::forArrayCellField(
            [&database, &j](uint32_t i) { return database.at(i).fields[j]; },
            database.length, "record.field#" + std::to_string(j)
        ) << std::endl;
    }
    */
}

float32_t** GetFields(
        const Database& database,
        const uint32_t dimension) {
    float32_t** fields = MallocMatrix(database.length, dimension);
    
    for (uint32_t i = 0; i < database.length; i++) {
        Record r = database.records[i];
        fields[i] = r.fields;
    }    

    return fields;
}

void SetFields(
        Database& database,
        float32_t** fields,
        const uint32_t n_component) {

    for (uint32_t i = 0; i < database.length; i++) {
        for (uint32_t j = 0; j < n_component; j++) {
            database.records[i].fields[j] = fields[i][j];
        }
    }    
}

const float32_t** ReduceDimensionality(
        Database& database, 
        const uint32_t final_dimension) {
    const float32_t** prj_matrix = GenerateProjectionMatrix(final_dimension, vector_num_dimension);
    
    RandomProjectionOnDataset(database, vector_num_dimension, final_dimension);

    return prj_matrix;
}
