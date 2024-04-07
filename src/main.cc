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
    std::string database_path = "contest-data-release-1m.bin";
    std::string query_set_path = "contest-queries-release-1m.bin";
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

    auto start = std::chrono::high_resolution_clock::now();

    Workflow(database_path, query_set_path, output_path);

    auto end = std::chrono::high_resolution_clock::now();

    std::cout 
        << "# ETA (ms): " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()  
        << std::endl;
}
