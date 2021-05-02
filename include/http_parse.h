#pragma once

#include <string>
#include <vector>

#include "http_base.h"

class HttpRequest : public HttpBase {
 public:
    HttpRequest() = default;
    HttpRequest(const int fd);
    std::string &get_method();
    std::string get_method() const;
    void set_method(const std::string &method);
    int read_line(const int fd, char *buffer);
 private:
    std::string method_;
    std::string url_;
    static const int buf_size_ = 4096;
    static const int read_size_ = 256;
};
