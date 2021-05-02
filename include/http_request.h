#pragma once

#include <string>
#include <vector>

#include "http_base.h"

class HttpRequest : public HttpBase {
 public:
    HttpRequest() = default;
    HttpRequest(const int fd);
    explicit HttpRequest(const HttpRequest &other) = default;
    HttpRequest &operator=(const HttpRequest &other) = default;
    ~HttpRequest() = default;

    std::string &get_method();
    std::string get_method() const;

    void set_method(const std::string &method);

 private:
    std::string method_;
    std::string url_;
};
