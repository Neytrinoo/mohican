#include <algorithm>

#include "string_to_lower.h"

void string_to_lower(std::string &str) {
    std::transform(str.begin(),
                   str.end(),
                   str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}
