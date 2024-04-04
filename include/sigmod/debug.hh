#ifndef DEBUG_HH
#define DEBUG_HH

#include <iostream>

inline void Debug(const std::string s) {
    std::cout << "# DEBUG | " << s << std::endl;
}

inline void Panic(const std::string s) {
    std::cout << "# PANIC | " << s << std::endl;
    exit(1);
}

void LogTime(const std::string s);

#endif
