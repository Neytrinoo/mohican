#pragma once

#include <vector>

#include "http_header.h"

class HttpBase {
 public:
    HttpBase();
    HttpBase(const std::vector<HttpHeader> &headers, int major = 0, int minor = 0);
    ~HttpBase() = default;
    void set_version(int major, int minor);
    void set_headers(std::vector<HttpHeader> headers);
    int &get_minor();
    int get_minor() const;
    int &get_major();
    int get_major() const;
 protected:
    int version_major_;
    int version_minor_;
    std::vector<HttpHeader> request_headers_;
};
