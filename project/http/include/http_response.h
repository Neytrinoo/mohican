#pragma once

#include <string>
#include <unordered_map>

#include "http_defines.h"
#include "http_base.h"

class HttpResponse : public HttpBase {
 public:
    HttpResponse() = default;
    HttpResponse(std::unordered_map<std::string, std::string> headers,
                 int major,
                 int minor,
                 int status,
                 const std::string &message);
    HttpResponse(const HttpResponse &other) = default;
    HttpResponse &operator=(const HttpResponse &other) = default;
    HttpResponse(HttpResponse &&other) = default;
    HttpResponse &operator=(HttpResponse &&other) = default;

    std::string get_string();

    int get_status() const;

    void add_line(const std::string &line);

 private:
    void add_first_line(const std::string &line);
    void add_header(const std::string &line);
 private:
    int status_ = 0;
    std::string message_;
    bool first_line_added_ = false;
    bool headers_read_ = false;
    bool request_ended_ = false;
};