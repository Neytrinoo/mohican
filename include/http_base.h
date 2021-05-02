#pragma once

#include <vector>

#include "http_header.h"

class HttpBase {
 public:
    HttpBase();
    HttpBase(std::vector<HttpHeader> &headers, int major = 1, int minor = 0);
    ~HttpBase() = default;
    void set_version(int major, int minor);
    void set_headers(std::vector<HttpHeader> headers);
    int &get_minor();
    int get_minor() const;
    int &get_major();
    int get_major() const;
 protected:
    int http_version_major_;
    int http_version_minor_;
    std::vector<HttpHeader> http_headers_;
};
