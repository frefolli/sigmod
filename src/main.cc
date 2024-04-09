#include <chrono>
#include <filesystem>
#include <sigmod.hh>
#include <iostream>
#include <stdexcept>

void assert_file_exists(std::string path, std::string what) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error(what + ": path '" + path + "' doesn't exists");
  }
}

int main(int argc, char** args) {
    std::srand(std::time(0));

    std::string database_path = "dummy-data.bin";
    std::string query_set_path = "dummy-queries.bin";
    std::string output_path = "output.bin";

    if (argc > 1) {
        database_path = std::string(args[1]);
    }

    if (argc > 2) {
        query_set_path = std::string(args[2]);
    }

    if (argc > 3) {
        output_path = std::string(args[3]);
    }

    assert_file_exists(database_path, "database_path");
    assert_file_exists(query_set_path, "query_set_path");
    
    #ifdef COMPARE_SOLUTIONS

    std::string first_solutions[] = {
        "output-exaustive.bin",
        "output-ball-forest.bin",
        "output-kd-forest.bin",
        "output-vp-forest.bin"
    };

    std::string second_solutions[] = {
        "output-red-10d-exaustive.bin",
        "output-red-10d-ball-forest.bin",
        "output-red-10d-kd-forest.bin",
        "output-red-10d-vp-forest.bin",
    };

    for (uint32_t i = 0; i < sizeof(first_solutions)/sizeof(first_solutions[0]); i++) {
        assert_file_exists(first_solutions[i], "first_solutions");
        assert_file_exists(second_solutions[i], "second_solutions");
    
        std::cout << "Recall between " << first_solutions[i] << " - " 
            << second_solutions[i] << " := " 
            << CompareSolutionsFromFiles(first_solutions[i], second_solutions[i], TOT_ELEMENTS)
            << std::endl;
    }

    #else

    auto start = std::chrono::high_resolution_clock::now();

    Workflow(database_path, query_set_path, output_path);

    auto end = std::chrono::high_resolution_clock::now();

    std::cout 
        << "# ETA (ms): " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()  
        << std::endl;
        
    #endif
}
