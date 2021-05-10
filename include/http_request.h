#pragma once

#include <string>

#include "http_base.h"

class HttpRequest : public HttpBase {
 public:
    HttpRequest() = default;
    HttpRequest(const std::string &str);
    explicit HttpRequest(const HttpRequest &other) = default;
    HttpRequest &operator=(const HttpRequest &other) = default;
    ~HttpRequest() = default;

    std::string &get_method();
    std::string get_method() const;
    std::string &get_url();
    std::string get_url() const;

 private:
    std::string method_;
    std::string url_;
};
