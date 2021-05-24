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

    void add_line(const std::string &line);

 private:
    void add_first_line(const std::string &line);
    void add_header(const std::string &line);

 private:
    std::string method_;
    std::string url_;
    bool first_line_added_ = false;
    bool headers_read_ = false;
    bool request_ended_ = false;
};
