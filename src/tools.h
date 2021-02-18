#pragma once

#include <string>
#include <sstream>
#include <iostream>

inline int toInt(const std::string& _string) {
    int x = 0;
    std::istringstream cur(_string);
    cur >> x;
    return x;
}
