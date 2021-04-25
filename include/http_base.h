#pragma once

#include <vector>

#include "http_header.h"

class HttpBase {
 public:
    HttpBase();
    HttpBase(std::vector<HttpHeader> &headers, int major = 1, int minor = 0);
    ~HttpBase() = default;
    void setVersion(int major, int minor);
    void setHeaders(std::vector<HttpHeader> headers);
    int &getMinor();
    int getMinor() const;
    int &getMajor();
    int getMajor() const;
 protected:
    int http_version_major_;
    int http_version_minor_;
    std::vector<HttpHeader> http_headers_;
};
