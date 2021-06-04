#include <unistd.h>

#include "http_base.h"

HttpBase::HttpBase(const std::unordered_map<std::string, std::string> &headers, int major, int minor)
        : headers_(headers), version_major_(major), version_minor_(minor) {}

int HttpBase::get_minor() const {
    return version_minor_;
}

int HttpBase::get_major() const {
    return version_major_;
}

std::unordered_map<std::string, std::string> &HttpBase::get_headers() {
    return headers_;
}
