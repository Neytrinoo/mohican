#pragma once

#include <string>
#include <unordered_map>

#include "defines.h"
#include "http_base.h"
#include "http_request.h"

class HttpResponse : public HttpBase {
 public:
    HttpResponse() = default;
    HttpResponse(std::unordered_map<std::string, std::string> headers,
                 int major,
                 int minor,
                 const std::string &filename,
                 int status,
                 const std::string &message);
    HttpResponse(const HttpResponse &other) = default;
    HttpResponse &operator=(const HttpResponse &other) = default;
    HttpResponse(HttpResponse &&other) = default;
    HttpResponse &operator=(HttpResponse &&other) = default;

    std::string get_string();

 private:
    int status_ = 0;
    std::string message_;
};