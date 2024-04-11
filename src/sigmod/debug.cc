#include <sigmod/debug.hh>
#include <chrono>

auto SIGMOD_LOG_TIME = std::chrono::high_resolution_clock::now();

void LogTime(const std::string s) {
    auto now = std::chrono::high_resolution_clock::now();
    std::cout << "# TIME | " << s << " | el. "
        << std::chrono::duration_cast<std::chrono::milliseconds>(now - SIGMOD_LOG_TIME).count()
        << " ms" << std::endl;
    SIGMOD_LOG_TIME = now;
}

std::string BytesToString(long long bytes) {
    std::string rep = "";
    static const long long KILO = 1024;
    static const long long MEGA = KILO*1024;
    static const long long GIGA = MEGA*1024;
    static const long long TERA = GIGA*1024;
    if (bytes > TERA) {
        rep = std::to_string(bytes/TERA) + " TB";
    } else if (bytes > GIGA) {
        rep = std::to_string(bytes/GIGA) + " GB";
    } else if (bytes > MEGA) {
        rep = std::to_string(bytes/MEGA) + " MB";
    } else if (bytes > KILO) {
        rep = std::to_string(bytes/KILO) + " KB";
    } else {
        rep = std::to_string(bytes) + " B";
    }
    return rep;
}

long long SIGMOD_MEMORY_TRACKER = 0;
long long SIGMOD_DISTANCE_COMPUTATIONS = 0;

void LogMemory(const std::string s) {
    std::cout << "# MEMORY | " << s << " | Allocated " << BytesToString(SIGMOD_MEMORY_TRACKER) << std::endl;
    SIGMOD_MEMORY_TRACKER = 0;
}
