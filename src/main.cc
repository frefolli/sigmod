#include <chrono>
#include <sigmod.hh>
#include <iostream>

int main(int argc, char** args) {
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

    auto start = std::chrono::high_resolution_clock::now();

    Workflow(database_path, query_set_path, output_path);

    auto end = std::chrono::high_resolution_clock::now();

    std::cout 
        << "ETA (ms): " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()  
        << std::endl;
}
