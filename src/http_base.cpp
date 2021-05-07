#include <unistd.h>

#include "http_base.h"

HttpBase::HttpBase(const std::vector<HttpHeader> &headers, int major, int minor)
        : old_headers_(headers), version_major_(major), version_minor_(minor) {}

int HttpBase::get_minor() const {
    return version_minor_;
}

int HttpBase::get_major() const {
    return version_major_;
}

int HttpBase::read_line(const int fd, char *buffer) {
    int i = 0;
    while (i < buf_size_) {
        char c;
        int r = read(fd, &c, sizeof c);
        if (r <= 0)
            return -1;
        if (c == '\n')
            break;
        buffer[i++] = c;
    }
    if (i > 0 && buffer[i - 1] == '\r')
        i--;
    buffer[i] = '\0';
    return i;
}

std::unordered_map<std::string, std::string> &HttpBase::get_headers() {
    return headers_;
}
