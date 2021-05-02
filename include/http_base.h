#pragma once

#include <vector>

#include "http_header.h"

class HttpBase {
 public:
    HttpBase() = default;
    HttpBase(const std::vector<HttpHeader> &headers, int major = 0, int minor = 0);
    explicit HttpBase(const HttpBase &other) = default;
    HttpBase &operator=(const HttpBase &other) = default;
    ~HttpBase() = default;

    void set_version(int major, int minor);
    void set_headers(std::vector<HttpHeader> headers);

    int &get_minor();
    int get_minor() const;
    int &get_major();
    int get_major() const;

 protected:
    int read_line(const int fd, char *buffer);

 protected:
    int version_major_;
    int version_minor_;
    std::vector<HttpHeader> headers_;
    static const int buf_size_ = 4096;
    static const int read_size_ = 256;
};
